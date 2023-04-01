/*
  ==============================================================================

	PresetManager.cpp
	Created: 17 Apr 2021 12:08:47pm
	Author:  bkupe

  ==============================================================================
*/

#include "Preset/PresetIncludes.h"
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
	prevPreset(nullptr),
	currentPreset(nullptr)
{
	saveCurrentTrigger = addTrigger("Save Current", "Save state to current preset");
	loadCurrentTrigger = addTrigger("Load Current", "Load state to current preset");

	nextPresetTrigger = addTrigger("Next", "Load next preset");
	previousPresetTrigger = addTrigger("Previous", "Load previous preset");

	presetOnFileLoad = addTargetParameter("Preset On File Load", "Preset to load when the file is loaded", this);
	presetOnFileLoad->targetType = TargetParameter::CONTAINER;
	presetOnFileLoad->typesFilter.add(Preset::getTypeStringStatic());

	curPresetName = addStringParameter("Current Preset", "The name of the current preset, for reference", "");
	curPresetName->setControllableFeedbackOnly(true);

	Engine::mainEngine->addEngineListener(this);
	OSCRemoteControl::getInstance()->addRemoteControlListener(this);
}

RootPresetManager::~RootPresetManager()
{
	if (Engine::mainEngine != nullptr) Engine::mainEngine->removeEngineListener(this);
	if (OSCRemoteControl::getInstanceWithoutCreating()) OSCRemoteControl::getInstance()->removeRemoteControlListener(this);

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

	if (currentPreset != nullptr && !currentPreset->isClearing)
	{
		currentPreset->removeInspectableListener(this);
		currentPreset->isCurrent->setValue(false);
	}

	stopThread(100);
	prevPreset = currentPreset;
	currentPreset = p;

	if (currentPreset != nullptr)
	{
		currentPreset->addInspectableListener(this);
		currentPreset->isCurrent->setValue(true);
		if (currentPreset->transitionTime->floatValue() == 0)
		{
			bool recursive = true;

			if (currentPreset->noParentOnNearbyLoad->boolValue()
				&& prevPreset != nullptr
				&& prevPreset->parentContainer == currentPreset->parentContainer) recursive = false;

			currentPreset->load(recursive);
		}
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
	else if (t == nextPresetTrigger)
	{
		loadNextPreset(currentPreset, true);
	}
	else if (t == previousPresetTrigger)
	{
		loadPreviousPreset(currentPreset);
	}
}

void RootPresetManager::toggleControllablePresettable(Controllable* c)
{
	if (c == nullptr) return;
	if (isControllablePresettable(c))
	{
		AlertWindow* window = new AlertWindow("Remove from presets", "Do you want to remove all presets related to this parameter ?", AlertWindow::AlertIconType::QuestionIcon);
		window->addButton("Yes", 1, KeyPress(KeyPress::returnKey));
		window->addButton("No", 2);
		window->addButton("Cancel", 0, KeyPress(KeyPress::escapeKey));

		window->enterModalState(true, ModalCallbackFunction::create([window, c, this](int result)
			{
				if (result == 0) return;

				if (result == 1)
				{
					RootPresetManager::getInstance()->removeAllPresetsFor(c);
				}

				c->customData = var();

			}
		), true);
	}
	else
	{
		c->customData = var(new DynamicObject());
		c->customData.getDynamicObject()->setProperty("presettable", true);
		c->saveCustomData = true;
		if (c->type != Controllable::TRIGGER) ((Parameter*)c)->isOverriden = true;
	}
}

var RootPresetManager::getControllablePresetOption(Controllable* c, String option, var defaultVal)
{
	if (c == nullptr) return defaultVal;
	if (c->customData.isObject()) return c->customData.getProperty(option, defaultVal);
	return defaultVal;
}

void RootPresetManager::setControllablePresetOption(Controllable* c, String option, var val)
{
	if (c == nullptr) return;
	if (c->customData.isObject()) c->customData.getDynamicObject()->setProperty(option, val);
}

bool RootPresetManager::isControllablePresettable(Controllable* c)
{
	if (c == nullptr) return false;
	return c->customData.getProperty("presettable", false);
}

void RootPresetManager::removeAllPresetsFor(Controllable* c)
{
	if (c == nullptr) return;
	Array<Preset*> presets = getAllPresets();
	for (auto& pr : presets)
	{
		pr->removeControllableFromDataMap(c);
	}
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


void RootPresetManager::fillPresetMenu(PopupMenu& menu, int indexOffset, Controllable* targetControllable = nullptr)
{
	Array<Preset*> presets = getAllPresets();
	int index = indexOffset;
	for (auto& p : presets) menu.addItem(index++, p->niceName, true, p->hasPresetControllable(targetControllable));
}

Preset* RootPresetManager::getPresetForMenuResult(int result)
{
	Array<Preset*> presets = getAllPresets();
	return presets[result];
}

void RootPresetManager::processMessage(const OSCMessage& m)
{
	StringArray addSplit;
	addSplit.addTokens(m.getAddressPattern().toString(), "/", "");
	if (addSplit.size() < 3) return;
	if (addSplit[1] != "presets") return;

	String action = addSplit[2];
	if (action == "loadPreset")
	{
		String presetName = m[0].getString();
		Array<Preset*> presets = getAllPresets();
		for (auto& p : presets) if (p->niceName == presetName || p->shortName == presetName) p->loadTrigger->trigger();
	}
}


void RootPresetManager::run()
{
	if (currentPreset == nullptr) return;

	bool recursive = true;
	if (currentPreset->noParentOnNearbyLoad->boolValue()
		&& prevPreset != nullptr
		&& prevPreset->parentContainer == currentPreset->parentContainer) recursive = false;

	var targetValues = currentPreset->getPresetValues(recursive);
	NamedValueSet props = targetValues.getDynamicObject()->getProperties();

	initTargetMap.clear();

	Preset::TransitionMode tm = currentPreset->directTransitionMode->getValueDataAsEnum<Preset::TransitionMode>();

	for (auto& val : props)
	{
		if (Parameter* tp = dynamic_cast<Parameter*>(Engine::mainEngine->getControllableForAddress(val.name.toString())))
		{
			if (!isControllablePresettable(tp)) continue;
			var initTargetValue;
			initTargetValue.append(tp->value);
			initTargetValue.append(val.value);

			bool canInterpolate = tp->type != Parameter::BOOL && tp->type != Parameter::ENUM && tp->type != Parameter::STRING && tp->type != Parameter::ENUM;
			int option = getControllablePresetOption(tp, "transition", canInterpolate ? Preset::INTERPOLATE : Preset::DEFAULT);
			if (option == Preset::DEFAULT) option = tm;
			initTargetValue.append(option);
			initTargetMap.set(tp, initTargetValue);
		}
	}

	float progression = 0;


	process(0, currentPreset->transition.getValueAtPosition(0)); //force for at_start changes

	double timeAtStart = Time::getMillisecondCounter() / 1000.0;
	float totalTime = currentPreset->transitionTime->floatValue();

	while (!threadShouldExit())
	{
		double currentTime = Time::getMillisecondCounter() / 1000.0 - timeAtStart;
		progression = jmin<float>(currentTime / totalTime, 1);
		float weight = currentPreset->transition.getValueAtPosition(progression);
		process(progression, weight);
		if (progression == 1) return;
		wait(30); //~30fps
	}

}

void RootPresetManager::process(float progression, float weight)
{
	HashMap<WeakReference<Controllable>, var>::Iterator it(initTargetMap);

	while (it.next())
	{
		if (it.getKey().wasObjectDeleted()) continue;
		Controllable* c = it.getKey().get();

		Preset::TransitionMode itm = (Preset::TransitionMode)(int)it.getValue()[2];
		if ((itm == Preset::AT_START && progression != 0) || (itm == Preset::AT_END && progression != 1)) continue;


		if (c->type == Controllable::TRIGGER)
		{
			if (itm == Preset::AT_START || itm == Preset::AT_END) ((Trigger*)c)->trigger();
		}
		else
		{
			Parameter* p = (Parameter*)c;
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
}

void RootPresetManager::endLoadFile()
{
	if (presetOnFileLoad->targetContainer != nullptr)
	{
		setCurrentPreset((Preset*)presetOnFileLoad->targetContainer.get());
	}
}
