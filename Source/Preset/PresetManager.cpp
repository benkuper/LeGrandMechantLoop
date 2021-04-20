/*
  ==============================================================================

	PresetManager.cpp
	Created: 17 Apr 2021 12:08:47pm
	Author:  bkupe

  ==============================================================================
*/

#include "PresetManager.h"

juce_ImplementSingleton(RootPresetManager);

PresetManager::PresetManager() :
	BaseManager("Presets")
{
	itemDataType = "Preset";
}

PresetManager::~PresetManager()
{
}

//Root
RootPresetManager::RootPresetManager() :
	PresetManager(),
	Thread("Preset Manager"),
	currentPreset(nullptr),
	transition("Transition Curve")
{
	saveCurrentTrigger = addTrigger("Save Current", "Save state to current preset");
	loadCurrentTrigger = addTrigger("Load Current", "Load state to current preset");
	transitionTime = addFloatParameter("Transition Time", "Time to transition.", 0, 0);
	transitionTime->defaultUI = FloatParameter::TIME;

	directTransitionMode = addEnumParameter("Direct Transition Mode", "For Boolean, String, Enum and other non transitionnable parameters.");
	directTransitionMode->addOption("Change at start", AT_START)->addOption("Change at end", AT_END);
	transition.addKey(0, 0);
	transition.addKey(1, 1);
	addChildControllableContainer(&transition);
}

RootPresetManager::~RootPresetManager()
{
	stopThread(100);
	setCurrentPreset(nullptr);
}

void RootPresetManager::clear()
{
	setCurrentPreset(nullptr);
	BaseManager::clear();
}

void RootPresetManager::setCurrentPreset(Preset* p)
{
	//if (currentPreset == p) return; // commented to be able to reload

	if (currentPreset != nullptr)
	{
		currentPreset->removeInspectableListener(this);
		currentPreset->isCurrent->setValue(false);
	}

	stopThread(100);
	currentPreset = p;

	if (currentPreset != nullptr)
	{
		currentPreset->addInspectableListener(this);
		currentPreset->isCurrent->setValue(true);
		if (transitionTime->floatValue() == 0) currentPreset->load();
		else startThread();
	}
}

void RootPresetManager::onContainerTriggerTriggered(Trigger* t)
{
	if (t == saveCurrentTrigger)
	{
		if (currentPreset != nullptr) currentPreset->saveTrigger->trigger();
	}
	else if (t == loadCurrentTrigger)
	{
		if (currentPreset != nullptr) currentPreset->loadTrigger->trigger();
	}
}

void RootPresetManager::toggleParameterPresettable(Parameter* p)
{
	if (p == nullptr) return;
	if (isParameterPresettable(p)) p->customData = var();
	else
	{
		p->customData = var(new DynamicObject());
		p->customData.getDynamicObject()->setProperty("presettable", true);
		p->isOverriden = true;
	}
}

var RootPresetManager::getParameterPresetOption(Parameter* p, String option, var defaultVal)
{
	if (p == nullptr) return defaultVal;
	if (p->customData.isObject()) return p->customData.getProperty(option, defaultVal);
	return defaultVal;
}

void RootPresetManager::setParameterPresetOption(Parameter* p, String option, var val)
{
	if (p == nullptr) return;
	if (p->customData.isObject()) p->customData.getDynamicObject()->setProperty(option, val);
}

bool RootPresetManager::isParameterPresettable(Parameter* p)
{
	if (p == nullptr) return false;
	return p->customData.getProperty("presettable", false);
}

void RootPresetManager::inspectableDestroyed(Inspectable* i)
{
	if (i == currentPreset) setCurrentPreset(nullptr);
}

Array<Preset*> RootPresetManager::getAllPresets(Preset * parent)
{
	PresetManager* pm = parent == nullptr ? RootPresetManager::getInstance() : parent->subPresets.get();
	Array<Preset*> result;
	if(parent != nullptr) result.add(parent);
	for (auto& sp : pm->items) result.addArray(getAllPresets(sp));

	return result;
}


void RootPresetManager::fillPresetMenu(PopupMenu& menu, int indexOffset, Parameter* targetParam)
{
	Array<Preset*> presets = getAllPresets();
	int index = indexOffset;
	for (auto& p : presets) menu.addItem(index++, p->niceName, true, p->hasPresetParam(targetParam));
}

Preset * RootPresetManager::getPresetForMenuResult(int result)
{
	Array<Preset*> presets = getAllPresets();
	return presets[result];
}

var RootPresetManager::getJSONData()
{
	var data = BaseManager::getJSONData();
	data.getDynamicObject()->setProperty(transition.shortName, transition.getJSONData());
	return data;
}

void RootPresetManager::loadJSONDataManagerInternal(var data)
{
	BaseManager::loadJSONDataManagerInternal(data);
	transition.loadJSONData(transition.shortName);
}

void RootPresetManager::run()
{
	if (currentPreset == nullptr) return;
	var targetValues = currentPreset->getPresetValues();
	NamedValueSet props = targetValues.getDynamicObject()->getProperties();

	initTargetMap.clear();

	TransitionMode tm = directTransitionMode->getValueDataAsEnum<TransitionMode>();

	for (auto& val : props)
	{
		if (Parameter* tp = dynamic_cast<Parameter*>(Engine::mainEngine->getControllableForAddress(val.name.toString())))
		{
			if (!isParameterPresettable(tp)) continue;
			var initTargetValue;
			initTargetValue.append(tp->value);
			initTargetValue.append(val.value);

			bool canInterpolate = tp->type != Parameter::BOOL && tp->type != Parameter::ENUM && tp->type != Parameter::STRING && tp->type != Parameter::ENUM;
			int option = getParameterPresetOption(tp, "transition", canInterpolate ? INTERPOLATE : DEFAULT);
			if (option == DEFAULT) option = tm;
			initTargetValue.append(option);
			initTargetMap.set(tp, initTargetValue);
		}
	}

	float progression = 0;


	lerpParameters(0, transition.getValueAtPosition(0)); //force for at_start changes

	double timeAtStart = Time::getMillisecondCounter() / 1000.0;
	float totalTime = transitionTime->floatValue();

	while (!threadShouldExit())
	{
		double currentTime = Time::getMillisecondCounter() / 1000.0 - timeAtStart;
		progression = jmin<float>(currentTime / totalTime, 1);
		float weight = transition.getValueAtPosition(progression);
		lerpParameters(progression, weight);
		if (progression == 1) return;
		wait(30); //~30fps
	}

}

void RootPresetManager::lerpParameters(float progression, float weight)
{
	HashMap<WeakReference<Parameter>, var>::Iterator it(initTargetMap);

	while (it.next())
	{
		if (it.getKey().wasObjectDeleted()) continue;
		Parameter* p = it.getKey().get();

		TransitionMode itm = (TransitionMode)(int)it.getValue()[2];
		if ((itm == AT_START && progression != 0) || (itm == AT_END && progression != 1)) continue;

		if (itm == AT_START || itm == AT_END) p->setValue(it.getValue()[1]);
		else
		{
			if (p->isComplex())
			{
				var newVal;
				for (int i = 0; i < p->value.size(); i++)
				{
					newVal.append(jmap<float>(weight, it.getValue()[0][i], it.getValue()[1][i]));
					p->setValue(newVal);
				}
			}
			else
			{
				p->setValue(jmap<float>(weight, it.getValue()[0], it.getValue()[1]));
			}
		}
	}
}
