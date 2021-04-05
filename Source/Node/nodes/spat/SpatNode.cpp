/*
  ==============================================================================

    SpatNode.cpp
    Created: 15 Nov 2020 8:42:35am
    Author:  bkupe

  ==============================================================================
*/

#include "SpatNode.h"
#include "ui/SpatNodeViewUI.h"

SpatNode::SpatNode(var params) :
    Node(getTypeString(), params, true, true, true, true),
    fadeCurve("Fade Curve",nullptr),
    spatCC("Points")
{
	saveAndLoadRecursiveData = true;

	spatMode = addEnumParameter("Mode", "Mode of the spat");
	spatMode->addOption("Circle", CIRCLE)->addOption("Free", FREE);

	spatPosition = addPoint2DParameter("Position", "Position to compute the weights from");
	spatPosition->setBounds(0, 0, 1, 1);
	spatPosition->setPoint(.5f, .5f);

	spatRadius = addFloatParameter("Radius", "Radius to compute the weights from", .5f, 0, 1);
	
	AutomationKey * k = fadeCurve.addKey(0,1);
	k->easingType->setValueWithData(Easing::BEZIER);
	fadeCurve.addKey(1, 0);
	addChildControllableContainer(&fadeCurve);

	circleRadius = addFloatParameter("Circle Radius", "Radius of the circle if in circle mode", .8f, 0, 2);
	circleAngle = addFloatParameter("Circle Angle", "Angle of the circle if in cirlce mode", 0, 0, 360);
	
	addChildControllableContainer(&spatCC);

	viewUISize->setPoint(300, 300);
}

void SpatNode::updateAudioOutputsInternal()
{
    updateSpatPoints();
}

void SpatNode::updateSpatPoints()
{
	jassert(processor->isSuspended());
	DBG("Num spat items before updates : " << spatCC.controllableContainers.size() << " / " << getNumAudioOutputs());

	while (spatCC.controllableContainers.size() > getNumAudioOutputs())
	{
		spatCC.removeChildControllableContainer(spatCC.controllableContainers[spatCC.controllableContainers.size() - 1]);
	}

	while (spatCC.controllableContainers.size() < getNumAudioOutputs())
	{
		int index = spatCC.controllableContainers.size();
		SpatItem* si = new SpatItem(index);
		spatCC.addChildControllableContainer(si, true);
	}

	placeItems();
	updateRadiuses();


	jassert(processor->isSuspended());

	DBG("Num spat items after updates : " << spatCC.controllableContainers.size() << " / " << getNumAudioOutputs());
}

void SpatNode::placeItems()
{
	SpatMode m = spatMode->getValueDataAsEnum<SpatMode>();
	switch (m)
	{
	case CIRCLE:
    {
        int numItems = spatCC.controllableContainers.size();
        for (int i = 0; i < numItems; i++)
        {
            float rel = i * 1.0f / numItems;
            float rad = circleRadius->floatValue();

            float angle = (circleAngle->floatValue() + rel * 360 + 180) * float_Pi / 180;
            SpatItem* si = (SpatItem*)spatCC.controllableContainers[i].get();
            si->position->setPoint(.5f + cosf(angle) * .5f * rad, .5f + sinf(angle) * .5f * rad);
        }
    }
	break;
            
    default:
        break;
	}

}

void SpatNode::updateRadiuses()
{
	int numItems = spatCC.controllableContainers.size();
	for (int i = 0; i < numItems; i++)
	{
		SpatItem* si = (SpatItem*)spatCC.controllableContainers[i].get();
		si->radius->defaultValue = spatRadius->floatValue();
		if (!si->radius->isOverriden) si->radius->resetValue();
	}
}

void SpatNode::onContainerParameterChangedInternal(Parameter* p)
{
	Node::onContainerParameterChangedInternal(p);

	if (p == spatMode || p == circleRadius || p == circleAngle)
	{
		placeItems();
	}
	else if (p == spatRadius)
	{
		updateRadiuses();
	}
}

void SpatNode::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
	int numSamples = buffer.getNumSamples();

	
	jassert(getNumAudioOutputs() == spatCC.controllableContainers.size());

	int numOutputs = spatCC.controllableContainers.size();

	tmpBuffer.setSize(numOutputs, numSamples);
	tmpBuffer.clear();


	for (int outputIndex = 0; outputIndex < numOutputs; outputIndex++)
	{
		SpatItem* si = (SpatItem*)spatCC.controllableContainers[outputIndex].get();
		float dist = spatPosition->getPoint().getDistanceFrom(si->position->getPoint());
		float rad = si->radius->floatValue();
		float relDist = jlimit<float>(0, 1, rad == 0 ? 1 : dist / rad);
		float weight = fadeCurve.getValueAtPosition(relDist);

		for (int inputIndex = 0; inputIndex < getNumAudioInputs(); inputIndex++)
		{
			tmpBuffer.addFrom(outputIndex, 0, buffer.getReadPointer(inputIndex), numSamples, weight);
		}

		si->weight->setValue(weight);
	}

	for (int outputIndex = 0; outputIndex < numOutputs; outputIndex++)
	{
		buffer.clear(outputIndex, 0, numSamples);
		buffer.addFrom(outputIndex, 0, tmpBuffer.getReadPointer(outputIndex), numSamples);

		jassert(!processor->isSuspended()); 
		SpatItem* si = (SpatItem*)spatCC.controllableContainers[outputIndex].get();
		float rms = buffer.getRMSLevel(outputIndex, 0, buffer.getNumSamples());
		float curVal = si->rms->floatValue();
		float targetVal = si->rms->getLerpValueTo(rms, rms > curVal ? .8f : .2f);
		si->rms->setValue(targetVal);
	}
}

BaseNodeViewUI* SpatNode::createViewUI()
{
    return new SpatNodeViewUI(this);
}
