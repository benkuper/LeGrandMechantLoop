/*
  ==============================================================================

	PresetManager.cpp
	Created: 17 Apr 2021 12:08:47pm
	Author:  bkupe

  ==============================================================================
*/

#include "Preset/PresetIncludes.h"

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
	currentPreset(nullptr)
{
	saveCurrentTrigger = addTrigger("Save Current", "Save state to current preset");
	loadCurrentTrigger = addTrigger("Load Current", "Load state to current preset");

	nextPreset = addTrigger("Next", "Load next preset");
	previousPreset = addTrigger("Previous", "Load previous preset");

	curPresetName = addStringParameter("Current Preset", "The name of the current preset, for reference", "");
	curPresetName->setControllableFeedbackOnly(true);

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
		if (currentPreset->transitionTime->floatValue() == 0) currentPreset->load();
		else startThread();
	}

	curPresetName->setValue(currentPreset != nullptr ? currentPreset->niceName : "");
}

void RootPresetManager::loadNextPreset(Preset* p, bool recursive)
{
	if (p == nullptr) return;

	if (recursive && p->subPresets->items.size() > 0)
	{
		setCurrentPreset(p->subPresets->items.getFirst());
		return;
	}

	PresetManager* pm = p != nullptr ? (PresetManager*)p->parentContainer.get() : this;
	if (pm == nullptr) return;
	Preset* np = pm->items.indexOf(p) < pm->items.size() - 1 ? pm->items[pm->items.indexOf(p) + 1] : nullptr;
	if (np != nullptr)
	{
		if (np->skipInPrevNext->boolValue()) loadNextPreset(np, true);
		else setCurrentPreset(np);
	}
	else
	{
		Preset* pp = (Preset*)pm->parentContainer.get();
		if (pp == nullptr) return;
		loadNextPreset(pp, false);
	}
}

void RootPresetManager::loadPreviousPreset(Preset* p)
{
	PresetManager* pm = p != nullptr ? (PresetManager*)p->parentContainer.get() : this;
	if (pm == nullptr) return;

	Preset* prevP = pm->items.indexOf(p) > 0 ? pm->items[pm->items.indexOf(p) - 1] : nullptr;
	if (prevP != nullptr)
	{
		loadLastNestedPresetFor(prevP);
		return;
	}

	Preset* pp = ControllableUtil::findParentAs<Preset>(p);
	if (pp != nullptr)
	{
		if (pp->skipInPrevNext->boolValue()) loadPreviousPreset(pp);
		else setCurrentPreset(pp);
	}
}

void RootPresetManager::loadLastNestedPresetFor(Preset* p)
{
	if (p == nullptr) return;

	PresetManager* pm = p->subPresets.get();

	if (pm->items.size() == 0)
	{
		setCurrentPreset(p);
		return;
	}

	loadLastNestedPresetFor(pm->items.getLast());
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
	else if (t == nextPreset)
	{
		loadNextPreset(currentPreset, true);
	}
	else if (t == previousPreset)
	{
		loadPreviousPreset(currentPreset);
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

Array<Preset*> RootPresetManager::getAllPresets(Preset* parent)
{
	PresetManager* pm = parent == nullptr ? RootPresetManager::getInstance() : parent->subPresets.get();
	Array<Preset*> result;
	if (parent != nullptr) result.add(parent);
	for (auto& sp : pm->items) result.addArray(getAllPresets(sp));

	return result;
}


void RootPresetManager::fillPresetMenu(PopupMenu& menu, int indexOffset, Parameter* targetParam)
{
	Array<Preset*> presets = getAllPresets();
	int index = indexOffset;
	for (auto& p : presets) menu.addItem(index++, p->niceName, true, p->hasPresetParam(targetParam));
}

Preset* RootPresetManager::getPresetForMenuResult(int result)
{
	Array<Preset*> presets = getAllPresets();
	return presets[result];
}


void RootPresetManager::run()
{
	if (currentPreset == nullptr) return;
	var targetValues = currentPreset->getPresetValues();
	NamedValueSet props = targetValues.getDynamicObject()->getProperties();

	initTargetMap.clear();

	Preset::TransitionMode tm = currentPreset->directTransitionMode->getValueDataAsEnum<Preset::TransitionMode>();

	for (auto& val : props)
	{
		if (Parameter* tp = dynamic_cast<Parameter*>(Engine::mainEngine->getControllableForAddress(val.name.toString())))
		{
			if (!isParameterPresettable(tp)) continue;
			var initTargetValue;
			initTargetValue.append(tp->value);
			initTargetValue.append(val.value);

			bool canInterpolate = tp->type != Parameter::BOOL && tp->type != Parameter::ENUM && tp->type != Parameter::STRING && tp->type != Parameter::ENUM;
			int option = getParameterPresetOption(tp, "transition", canInterpolate ? Preset::INTERPOLATE : Preset::DEFAULT);
			if (option == Preset::DEFAULT) option = tm;
			initTargetValue.append(option);
			initTargetMap.set(tp, initTargetValue);
		}
	}

	float progression = 0;


	lerpParameters(0, currentPreset->transition.getValueAtPosition(0)); //force for at_start changes

	double timeAtStart = Time::getMillisecondCounter() / 1000.0;
	float totalTime = currentPreset->transitionTime->floatValue();

	while (!threadShouldExit())
	{
		double currentTime = Time::getMillisecondCounter() / 1000.0 - timeAtStart;
		progression = jmin<float>(currentTime / totalTime, 1);
		float weight = currentPreset->transition.getValueAtPosition(progression);
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

		Preset::TransitionMode itm = (Preset::TransitionMode)(int)it.getValue()[2];
		if ((itm == Preset::AT_START && progression != 0) || (itm == Preset::AT_END && progression != 1)) continue;

		if (itm == Preset::AT_START || itm == Preset::AT_END) p->setValue(it.getValue()[1]);
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
