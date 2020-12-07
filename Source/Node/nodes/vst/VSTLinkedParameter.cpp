/*
  ==============================================================================

	VSTLinkedParameter.cpp
	Created: 7 Dec 2020 4:54:15pm
	Author:  bkupe

  ==============================================================================
*/

#include "VSTLinkedParameter.h"
#include "ui/VSTLinkedParameterUI.h"


// Parameter Link
VSTParameterLink::VSTParameterLink(AudioProcessorParameter* vstParam, Parameter* param) :
	vstParam(vstParam),
	param(param),
	antiFeedback(false)
{
	vstParam->addListener(this);
}

VSTParameterLink::~VSTParameterLink()
{
	vstParam->removeListener(this);
}

void VSTParameterLink::updateFromVST(float value)
{
	if (antiFeedback) return;
	antiFeedback = true;
	param->setValue(value);
	antiFeedback = false;
}

void VSTParameterLink::updateFromParam(float value)
{
	if (antiFeedback) return;
	antiFeedback = true;
	vstParam->setValue(value);
	antiFeedback = false;
}

String VSTParameterLink::getNameFromParam() const
{
	return vstParam->getName(32);
}

String VSTParameterLink::getDescriptionFromParam() const
{
	return vstParam->getName(32) + (vstParam->getLabel().isNotEmpty() ? " (" + vstParam->getLabel() + ")" : "");
}

void VSTParameterLink::parameterValueChanged(int parameterIndex, float newValue)
{
	updateFromVST(newValue);
}

void VSTParameterLink::parameterGestureChanged(int parameterIndex, bool gestureIsStarting)
{
	if (gestureIsStarting) valueAtGestureStart = vstParam->getValue();
	else param->setUndoableValue(valueAtGestureStart, vstParam->getValue());
}


//FLOAT
VSTLinkedFloatParameter::VSTLinkedFloatParameter(AudioProcessorParameter* vstParam) :
	VSTParameterLink(vstParam, this),
	FloatParameter(getNameFromParam(), getDescriptionFromParam(), vstParam->getDefaultValue(), 0, 1)
{
	if (vstParam->getNumSteps() < 100)
	{
		unitSteps = vstParam->getNumSteps();
	}
}

VSTLinkedFloatParameter::~VSTLinkedFloatParameter()
{
}

void VSTLinkedFloatParameter::setValueInternal(var& value)
{
	FloatParameter::setValueInternal(value);
	updateFromParam(value);
}


VSTLinkedIntParameter::VSTLinkedIntParameter(AudioProcessorParameter* vstParam) :
	VSTParameterLink(vstParam, this),
	IntParameter(getNameFromParam(), getDescriptionFromParam(), vstParam->getDefaultValue(), 0)
{
}

void VSTLinkedIntParameter::setValueInternal(var& value)
{
	IntParameter::setValueInternal(value);
	updateFromParam(value);
}

VSTLinkedBoolParameter::VSTLinkedBoolParameter(AudioProcessorParameter* vstParam) :
	VSTParameterLink(vstParam, this),
	BoolParameter(getNameFromParam(), getDescriptionFromParam(), vstParam->getDefaultValue())
{
}

void VSTLinkedBoolParameter::setValueInternal(var& value)
{
	BoolParameter::setValueInternal(value);
	updateFromParam(value);
}



//CONTAINER

VSTParameterContainer::VSTParameterContainer(AudioPluginInstance* vst) :
	ControllableContainer("VST Parameters"),
	vst(vst)
{
	fillContainerForVSTParamGroup(this, &vst->getParameterTree()); //fill empty containers and create idContainerMap
}

VSTParameterContainer::~VSTParameterContainer()
{
}

void VSTParameterContainer::addAllParams()
{
	const Array<AudioProcessorParameter *> &params = vst->getParameters();
	for (auto p : params) addVSTParam(p);
	//fillContainerForVSTParamGroup(this, &vst->getParameterTree(), true);
}

void VSTParameterContainer::removeAllParams()
{
	idParamMap.clear();
	idContainerMap.clear();
	inspectableIdMap.clear();
	clear();
	fillContainerForVSTParamGroup(this, &vst->getParameterTree());

	queuedNotifier.addMessage(new ContainerAsyncEvent(ContainerAsyncEvent::ControllableContainerNeedsRebuild, this));
}

VSTParameterLink * VSTParameterContainer::addVSTParam(AudioProcessorParameter* vstP)
{
	if (vstP == nullptr)
	{
		DBG("VSTP is null !");
		jassertfalse;
		return nullptr;
	}

	int index = vstP->getParameterIndex();
	if (hasParam(index))
	{
		DBG("Already exists");
		return idParamMap[index];
	}

	if (idContainerMap.contains(index))
	{
		ControllableContainer* cc = idContainerMap[index];

		Parameter * p = createParameterForVSTParam(vstP);
		p->isRemovableByUser = true;
		p->setValue(vstP->getValue());
		idParamMap.set(index, (VSTParameterLink *)p);
		p->addInspectableListener(this);
		cc->addParameter(p);

		return (VSTParameterLink*)p;
	}

	return nullptr;
}


void VSTParameterContainer::fillContainerForVSTParamGroup(ControllableContainer* cc, const AudioProcessorParameterGroup* group/*, bool createParameters*/)
{
	Array<AudioProcessorParameter*> params = group->getParameters(false);
	for (auto& vstP : params)
	{
		/*if (createParameters)
		{
			Parameter* pp = createParameterForVSTParam(vstP);
			cc->addParameter(pp);
		}
		else
		{
			*/
		idContainerMap.set(vstP->getParameterIndex(), cc);
		//}
	}

	const Array<const AudioProcessorParameterGroup*> groups = group->getSubgroups(false);
	for (auto& g : groups)
	{
		ControllableContainer* childCC = cc->getControllableContainerByName(g->getName());
		if(childCC == nullptr) childCC = new ControllableContainer(g->getName());
		fillContainerForVSTParamGroup(childCC, g);
		cc->addChildControllableContainer(childCC, true);
	}
}

Parameter* VSTParameterContainer::createParameterForVSTParam(AudioProcessorParameter* vstParam)
{
	Parameter* p = nullptr;
	if (vstParam->isBoolean()) p = new VSTLinkedBoolParameter(vstParam);
	else if (vstParam->isDiscrete())
	{
		if (vstParam->getNumSteps() == 2) p = new VSTLinkedBoolParameter(vstParam);
		else p = new VSTLinkedIntParameter(vstParam);
	}
	else p = new VSTLinkedFloatParameter(vstParam);

	return p;

}

void VSTParameterContainer::inspectableDestroyed(Inspectable* i)
{
	if (inspectableIdMap.contains(i))
	{
		int index = inspectableIdMap[i];
		idParamMap.remove(index);
		inspectableIdMap.remove(i);
	}
}

var VSTParameterContainer::getJSONData()
{
	var data;

	const Array<AudioProcessorParameter*>& params = vst->getParameters();

	for (int i = 0; i < params.size(); i++)
	{
		var pData(new DynamicObject());
		bool isExposed = (idParamMap.contains(i));
		bool isOverriden = params[i]->getValue() != params[i]->getDefaultValue();
		if (isExposed || isOverriden)
		{
			pData.getDynamicObject()->setProperty("index", i);
			if (isExposed) pData.getDynamicObject()->setProperty("exposed", true);
			if (isOverriden) pData.getDynamicObject()->setProperty("value", params[i]->getValue());
			data.append(pData);
		}
	}

	return data;
}

void VSTParameterContainer::loadJSONData(var data, bool createIfNotThere)
{
	if (vst == nullptr) return;
	const Array<AudioProcessorParameter *> &params = vst->getParameters();

	for (int i = 0; i < data.size(); i++)
	{
		int index = data[i].getProperty("index", -1);
		if(data[i].hasProperty("value")) params[index]->setValue(data[i].getProperty("value", params[i]->getValue()));
		if (data[i].getProperty("exposed", false)) addVSTParam(params[index]);
	}
}

InspectableEditor* VSTParameterContainer::getEditor(bool isRoot)
{
	return new VSTParameterContainerEditor(this, isRoot);
}
