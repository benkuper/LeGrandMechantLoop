/*
  ==============================================================================

	PresetManager.cpp
	Created: 17 Apr 2021 12:08:47pm
	Author:  bkupe

  ==============================================================================
*/

#include "Preset/PresetIncludes.h"
#include "Transport/Transport.h"

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


	loadProgress = addFloatParameter("Load Progression", "Progression of the current preset loading", 0, 0, 1);
	loadProgress->setControllableFeedbackOnly(true);

	curPresetName = addStringParameter("Current Preset", "The name of the current preset, for reference", "");
	curPresetName->setControllableFeedbackOnly(true);

	curPresetDescription = addStringParameter("Current Preset Description", "The description of the current preset, for reference", "");
	curPresetDescription->multiline = true;
	curPresetDescription->setControllableFeedbackOnly(true);

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

	if (p != nullptr && !p->enabled->boolValue())
	{
		currentPreset = nullptr;
		return;
	}

	currentPreset = p;

	if (currentPreset != nullptr)
	{
		currentPreset->addInspectableListener(this);
		currentPreset->isCurrent->setValue(true);

		bool isDirectTransition = true;

		if (currentPreset->transitionCC.enabled->boolValue())
		{
			Transport::Quantization q = currentPreset->transitionQuantiz->getValueDataAsEnum<Transport::Quantization>();
			if (q == Transport::DEFAULT) q = Transport::getInstance()->quantization->getValueDataAsEnum<Transport::Quantization>();
			if (q == Transport::FREE) isDirectTransition = currentPreset->transitionTime->floatValue() == 0;
			else
			{
				if (Transport::getInstance()->isCurrentlyPlaying->boolValue())
				{
					isDirectTransition = false;
				}
				else
				{
					LOGWARNING("Preset transition set to quantized, but transport is not playing, switching to direct transition");
				}
			}
		}

		if (isDirectTransition)
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
	curPresetDescription->setValue(currentPreset != nullptr ? currentPreset->description->stringValue() : "");
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
		if (np->skipInPrevNext->boolValue() || !np->enabled->boolValue()) loadNextPreset(np, true);
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
		if (pp->skipInPrevNext->boolValue() || !pp->enabled->boolValue()) loadPreviousPreset(pp);
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



void RootPresetManager::fillPresetMenu(PopupMenu& menu, int indexOffset, Controllable* targetControllable, bool showValue, std::function<bool(Preset*, Controllable*)> tickCheckFunction)
{
	if (targetControllable == nullptr) return;

	Array<Preset*> presets = getAllPresets();
	int index = indexOffset;
	for (auto& p : presets)
	{
		String s = p->niceName;
		bool isChecked = tickCheckFunction != nullptr ? tickCheckFunction(p, targetControllable) : false;
		if (isChecked && showValue)
		{
			if (targetControllable->type != Controllable::TRIGGER)	s += " (" + p->dataMap[targetControllable].toString() + ")";
		}

		menu.addItem(index++, p->niceName, true, isChecked);
	}
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

	Preset::TransitionMode defaultTM = currentPreset->defaultTransitionMode->getValueDataAsEnum<Preset::TransitionMode>();

	for (auto& val : props)
	{
		if (Controllable* tc = dynamic_cast<Controllable*>(Engine::mainEngine->getControllableForAddress(val.name.toString())))
		{
			if (!isControllablePresettable(tc)) continue;
			var initTargetValue;
			initTargetValue.append(tc->type == Controllable::TRIGGER ? var() : ((Parameter*)tc)->value);
			initTargetValue.append(val.value);


			Preset::TransitionMode tm = defaultTM;

			if (currentPreset->transitionMap.contains(tc))
			{
				Preset::TransitionMode otm = currentPreset->transitionMap[tc];
				if (otm != Preset::DEFAULT) tm = otm;
			}

			bool canInterpolate = tc->type != Controllable::TRIGGER && tc->type != Parameter::BOOL && tc->type != Parameter::ENUM && tc->type != Parameter::STRING && tc->type != Parameter::ENUM;

			if (!canInterpolate && tm == Preset::INTERPOLATE) tm = Preset::AT_START;

			initTargetValue.append((int)tm);
			initTargetMap.set(tc, initTargetValue);
		}
	}

	float progression = 0;

	loadProgress->setValue(progression);

	process(0, currentPreset->transitionCurve.getValueAtPosition(0)); //force for at_start changes

	double timeAtStart = Time::getMillisecondCounter() / 1000.0;

	Transport::Quantization q = currentPreset->transitionQuantiz->getValueDataAsEnum<Transport::Quantization>();
	if (q == Transport::DEFAULT) q = Transport::getInstance()->quantization->getValueDataAsEnum<Transport::Quantization>();

	float totalTime = 0;
	int numBeatBar = currentPreset->numBeatBarQuantiz->intValue();
	switch (q)
	{
	case Transport::FREE: totalTime = currentPreset->transitionTime->floatValue(); break;;
	case Transport::BEAT:
		totalTime = Transport::getInstance()->getTimeToNextBeat() + Transport::getInstance()->getTimeForBeat(numBeatBar);
		break;
	case Transport::BAR:
		totalTime = Transport::getInstance()->getTimeToNextBar() + Transport::getInstance()->getTimeForBar(numBeatBar);
		break;
	case Transport::FIRSTLOOP: totalTime = Transport::getInstance()->getTimeToNextFirstLoop(); break;
	default:
		jassertfalse;
		break;
	}


	//LOG("Loading preset " << currentPreset->niceName << " in " << totalTime << " seconds.");

	while (!threadShouldExit())
	{
		double currentTime = Time::getMillisecondCounter() / 1000.0 - timeAtStart;
		progression = jmin<float>(currentTime / totalTime, 1);
		loadProgress->setValue(progression);

		float weight = currentPreset->transitionCurve.getValueAtPosition(progression);
		process(progression, weight);

		if (progression == 1)
		{
			loadProgress->setValue(0);
			return;
		}
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
