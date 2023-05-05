/*
  ==============================================================================

	Preset.cpp
	Created: 17 Apr 2021 12:08:52pm
	Author:  bkupe

  ==============================================================================
*/

#include "Preset/PresetIncludes.h"
#include "Transport/Transport.h"

Preset::Preset(var params) :
	BaseItem(getTypeString()),
	linkedPresetsCC("Linked Presets"),
	transitionCC("Transition"),
	transitionCurve("Transition Curve")
{
	itemDataType = "Preset";
	hideInEditor = true;

	subPresets.reset(new PresetManager());
	addChildControllableContainer(subPresets.get());
	subPresets->addBaseManagerListener(this);
	subPresets->hideInEditor = true;

	Random r;
	isCurrent = addBoolParameter("Is Current", "If this is the currently loaded preset", false);
	isCurrent->setControllableFeedbackOnly(true);
	isCurrent->hideInEditor = true;

	description = addStringParameter("Description", "Description of this preset", "");
	description->multiline = true;

	setHasCustomColor(true);
	itemColor->forceSaveValue = true;

	saveTrigger = addTrigger("Save", "Save the current state in this preset. If this is a sub-preset, this will only save overriden values");
	loadTrigger = addTrigger("Load", "Load the values in this preset. It this is a sub-preset, this will fetch all values up to its root preset");

	skipInPrev = addBoolParameter("Skip in Prev", "Skip this preset in the previous preset actions", false);
	skipInNext = addBoolParameter("Skip in Next", "Skip this preset in the next preset actions", false);
	noParentOnNearbyLoad = addBoolParameter("No Parent on Nearby Load", "If checked, this will prevent from loading the parent preset when loading this preset from a nearby preset", false);

	transitionQuantiz = transitionCC.addEnumParameter("Quantization", "Transition quantization for this preset");
	transitionQuantiz->addOption("Custom", Transport::FREE)->addOption("From Transport", Transport::DEFAULT)->addOption("First Loop", Transport::FIRSTLOOP)->addOption("Bar", Transport::BAR)->addOption("Beat", Transport::BEAT);

	numBeatBarQuantiz = transitionCC.addIntParameter("Quantiz Extra", "If quantization is set to beat or bar, this is the number of beats or bars to add to the current one for computing transition time", 0, 0);
	//numBeatBarQuantiz->setEnabled(false);

	transitionTime = transitionCC.addFloatParameter("Transition Time", "Time to transition.", 0, 0);
	transitionTime->defaultUI = FloatParameter::TIME;

	defaultTransitionMode = transitionCC.addEnumParameter("Default Transition Mode", "Default transition mode for all parameters. For non interpolable parameters, choosing Interpolate will result in Change at Start");
	defaultTransitionMode->addOption("Interpolate", INTERPOLATE)->addOption("Change at start", AT_START)->addOption("Change at end", AT_END);

	transitionCurve.addKey(0, 0);
	transitionCurve.addKey(1, 1);
	transitionCurve.editorCanBeCollapsed = true;
	transitionCurve.editorIsCollapsed = true;
	transitionCC.enabled->setDefaultValue(false);
	transitionCC.addChildControllableContainer(&transitionCurve);

	addChildControllableContainer(&transitionCC);

	linkedPresetsCC.userCanAddControllables = true;
	linkedPresetsCC.userAddControllablesFilters.add(TargetParameter::getTypeStringStatic());
	linkedPresetsCC.customUserCreateControllableFunc = [this](ControllableContainer* cc)
	{
		TargetParameter* p = cc->addTargetParameter("Preset 1", "Linked preset to load values from", RootPresetManager::getInstance());
		p->targetType = TargetParameter::CONTAINER;
		p->saveValueOnly = false;
		p->canBeDisabledByUser = true;
		p->isRemovableByUser = true;
		p->typesFilter.add(Preset::getTypeStringStatic());
	};

	addChildControllableContainer(&linkedPresetsCC);


	listUISize->isSavable = false;

	editorIsCollapsed = true;

	highlightLinkedInspectableOnSelect = true;

	Engine::mainEngine->addControllableContainerListener(this);
	Engine::mainEngine->addEngineListener(this);
}

Preset::~Preset()
{
	Engine::mainEngine->removeControllableContainerListener(this);
}

void Preset::clearItem()
{
	BaseItem::clearItem();
	HashMap<WeakReference<Controllable>, var>::Iterator it(dataMap);
	while (it.next()) if (it.getKey() != nullptr && !it.getKey().wasObjectDeleted())
	{
		it.getKey()->removeControllableListener(this);
	}
}

var Preset::getPresetValues(bool includeParents, Array<Controllable*> ignoreList, bool force)
{
	var data(new DynamicObject());

	if (!enabled->boolValue() && !force) return data;

	ignoreList.addArray(ignoredControllables);


	HashMap<WeakReference<Controllable>, var>::Iterator it(dataMap);
	while (it.next())
	{
		WeakReference<Controllable> c = it.getKey();
		if (c == nullptr || c.wasObjectDeleted()) continue;
		if (ignoreList.contains(c)) continue;

		String add = c->getControlAddress();
		if (!data.hasProperty(add))
		{
			var v;
			v.append(it.getValue());
			if(transitionMap.contains(c)) v.append(transitionMap[c]);
			data.getDynamicObject()->setProperty(add, v);
		}
		ignoreList.add(c);
	}


	//Parents and Linked presets
	Array<Preset*> presetsToInclude;
	if (includeParents)
	{
		if (parentContainer != nullptr && parentContainer != RootPresetManager::getInstance()) presetsToInclude.add((Preset*)parentContainer->parentContainer.get());

		for (auto& c : linkedPresetsCC.controllables)
		{
			if (!c->enabled) continue;
			Preset* p = dynamic_cast<Preset*>(((TargetParameter*)c)->targetContainer.get());
			if (p == nullptr) continue;
			presetsToInclude.add(p);
		}
	}

	for (auto& p : presetsToInclude)
	{
		var pData = p->getPresetValues(true, ignoreList);

		NamedValueSet props = pData.getDynamicObject()->getProperties();
		for (auto& p : props)
		{
			if (data.hasProperty(p.name)) continue;
			data.getDynamicObject()->setProperty(p.name, p.value);
		}
	}

	return data;
}

void Preset::saveContainer(ControllableContainer* container, bool recursive)
{
	Array<WeakReference<Parameter>> cList = container->getAllParameters(recursive);
	for (auto& c : cList) save(c);
}

void Preset::save(Controllable* controllable, bool saveAllPresettables, bool noCheck)
{
	if (controllable != nullptr)
	{
		if (!RootPresetManager::getInstance()->isControllablePresettable(controllable)) return;
		addControllableToDataMap(controllable);
	}
	else
	{
		if (!isCurrent->boolValue() && !noCheck)
		{
			AlertWindow::showOkCancelBox(AlertWindow::WarningIcon, "Not the current preset", "This is not the currently loaded preset. Do you want still want to save to preset", "Yes", "No", nullptr, ModalCallbackFunction::create([=](int result)
				{
					if (result) save(controllable, saveAllPresettables, true);
				}));

			return;
		}


		Array<WeakReference<Parameter>> params = Engine::mainEngine->getAllParameters(true);

		dataMap.clear();
		addressMap.clear();

		int numSaved = 0;
		for (auto& p : params)
		{
			if (!RootPresetManager::getInstance()->isControllablePresettable(p)) continue;
			var d = p->value;
			String add = p->getControlAddress();
			if (!p->shouldBeSaved()) continue;
			if (saveAllPresettables || overridenControllables.contains(add))
			{
				addControllableToDataMap(p);
				numSaved++;
			}
		}

		NLOG(niceName, "Saved " << numSaved << " values.");
	}
}


void Preset::load(bool recursive)
{
	int numLoaded = 0;
	var data = getPresetValues(recursive);
	NamedValueSet props = data.getDynamicObject()->getProperties();
	for (auto& p : props)
	{
		if (Controllable* tc = dynamic_cast<Controllable*>(Engine::mainEngine->getControllableForAddress(p.name.toString())))
		{
			if (!RootPresetManager::getInstance()->isControllablePresettable(tc)) continue;

			if (tc->type == Controllable::TRIGGER) ((Trigger*)tc)->trigger();
			else ((Parameter*)tc)->setValue(p.value[0]);

			numLoaded++;
		}
	}
	NLOG(niceName, "Loaded " << numLoaded << " values.");
}


void Preset::addControllableToDataMap(Controllable* c, var forceValue)
{
	if (c == nullptr) return;

	var val = c->type == Controllable::TRIGGER ? var() : forceValue.isVoid() ? ((Parameter*)c)->value : forceValue;

	String add = c->getControlAddress();

	dataMap.set(c, val);
	addressMap.set(add, val);
	controllableGhostAddressMap.set(c, add);
	lostControllables.remove(add);
	/*if (!isMain()) */overridenControllables.addIfNotAlreadyThere(add);

	c->addControllableListener(this);
	registerLinkedInspectable(c);
}

void Preset::updateControllableAddress(Controllable* c)
{
	if (c == nullptr) return;

	if (controllableGhostAddressMap.contains(c))
	{
		String oldAdd = controllableGhostAddressMap[c];
		var oldVal = addressMap[oldAdd];
		removeAddressFromDataMap(oldAdd);
		addControllableToDataMap(c, oldVal);
	}
}

void Preset::removeControllableFromDataMap(Controllable* c)
{
	if (c == nullptr) return;

	String add = c->getControlAddress();
	dataMap.remove(c);
	transitionMap.remove(c);
	controllableGhostAddressMap.remove(c);
	c->removeControllableListener(this);
	unregisterLinkedInspectable(c);
	addressMap.remove(add);
	lostControllables.remove(add);
	overridenControllables.removeAllInstancesOf(add);
}

void Preset::removeAddressFromDataMap(String address)
{
	if (Controllable* c = Engine::mainEngine->getControllableForAddress(address))
	{
		removeControllableFromDataMap(c);
		return;
	}

	controllableGhostAddressMap.removeValue(address);
	addressMap.remove(address);
	/* if (!isMain()) */ overridenControllables.removeAllInstancesOf(address);
}

void Preset::recoverLostControllables()
{
	HashMap<String, TransitionMode> initLostControllables;

	HashMap<String, TransitionMode>::Iterator it(lostControllables);
	while (it.next()) initLostControllables.set(it.getKey(), it.getValue()); //copy first because recovering changes the initial hashmap

	HashMap<String, TransitionMode>::Iterator lit(initLostControllables);
	while(lit.next())
	{
		String add = lit.getKey();
		if (Controllable* c = Engine::mainEngine->getControllableForAddress(add))
		{
			addControllableToDataMap(c, addressMap.contains(add) ? addressMap[add] : var());
			transitionMap.set(c, lit.getValue());
		}
	}
}

bool Preset::isMain()
{
	return parentContainer == RootPresetManager::getInstance();
}

bool Preset::hasPresetControllable(Controllable* c)
{
	return dataMap.contains(c);
}

void Preset::onContainerTriggerTriggered(Trigger* t)
{
	if (t == saveTrigger) save();
	else if (t == loadTrigger) RootPresetManager::getInstance()->setCurrentPreset(this);
}

void Preset::onControllableFeedbackUpdate(ControllableContainer* cc, Controllable* c)
{
	BaseItem::onControllableFeedbackUpdate(cc, c);

	if (c == transitionQuantiz)
	{
		Transport::Quantization q = transitionQuantiz->getValueDataAsEnum<Transport::Quantization>();
		transitionTime->setEnabled(q == Transport::FREE);
		numBeatBarQuantiz->setEnabled(q == Transport::BEAT || q == Transport::BAR);
	}
}

void Preset::itemAdded(Preset* p)
{
	if (!isCurrentlyLoadingData) p->itemColor->setColor(itemColor->getColor().brighter(.3f));
}

void Preset::controllableAdded(Controllable* c)
{
	BaseItem::controllableAdded(c);

	if (c->parentContainer == &linkedPresetsCC)
	{
		if (TargetParameter* p = (TargetParameter*)c)
		{
			p->targetType = TargetParameter::CONTAINER;
			p->saveValueOnly = false;
			p->canBeDisabledByUser = true;
			p->isRemovableByUser = true;
			p->typesFilter.add(Preset::getTypeStringStatic());
			p->setRootContainer(RootPresetManager::getInstance());
		}


	}
}

var Preset::getJSONData()
{
	var data = BaseItem::getJSONData();
	data.getDynamicObject()->setProperty("subPresets", subPresets->getJSONData());
	data.getDynamicObject()->setProperty("linkedPresets", linkedPresetsCC.getJSONData());
	data.getDynamicObject()->setProperty("values", getPresetValues(false, Array<Controllable*>(), true));
	data.getDynamicObject()->setProperty("transition", transitionCC.getJSONData());


	var transitionOverrideData(new DynamicObject());
	HashMap<WeakReference<Controllable>, TransitionMode>::Iterator it(transitionMap);
	while (it.next())
	{
		if (it.getKey().wasObjectDeleted()) continue;
		transitionOverrideData.getDynamicObject()->setProperty(it.getKey()->getControlAddress(), it.getValue());
	}

	data.getDynamicObject()->setProperty("transitionOverrides", transitionOverrideData);

	var ignoreData;
	for (auto& c : ignoredControllables) ignoreData.append(c->getControlAddress());
	data.getDynamicObject()->setProperty("ignores", ignoreData);

	var lostData(new DynamicObject());
	HashMap<String, TransitionMode>::Iterator lit(lostControllables);
	while (lit.next()) lostData.getDynamicObject()->setProperty(lit.getKey(), lit.getValue());
	data.getDynamicObject()->setProperty("lost", lostData);

	return data;
}

void Preset::loadJSONDataItemInternal(var data)
{
	subPresets->loadJSONData(data["subPresets"]);
	linkedPresetsCC.loadJSONData(data["linkedPresets"], true);
	transitionCC.loadJSONData(data["transition"], true);

	var transitionData = data["transitionOverrides"];

	var values = data["values"];
	if (values.isObject())
	{
		NamedValueSet params = values.getDynamicObject()->getProperties();
		for (auto& p : params)
		{
			if (Controllable* tp = dynamic_cast<Controllable*>(Engine::mainEngine->getControllableForAddress(p.name.toString())))
			{
				addControllableToDataMap(tp, p.value);
			}
			else
			{
				lostControllables.set(p.name.toString(), (TransitionMode)(int)transitionData.getProperty(p.name.toString(), TransitionMode::DEFAULT));
			}
		}
	}

	if (transitionData.isObject())
	{
		NamedValueSet tData = transitionData.getDynamicObject()->getProperties();
		for (auto& td : tData)
		{
			if (Controllable* tc = dynamic_cast<Controllable*>(Engine::mainEngine->getControllableForAddress(td.name.toString())))
			{
				transitionMap.set(tc, (TransitionMode)(int)td.value);
			}
		}
	}

	var ignoreData = data.getProperty("ignores", var());
	for (int i = 0; i < ignoreData.size(); i++)
	{
		if (Controllable* tc = dynamic_cast<Controllable*>(Engine::mainEngine->getControllableForAddress(ignoreData[i].toString())))
		{
			ignoredControllables.add(tc);
		}
	}

	var lostData = data.getProperty("lost", var());
	if (lostData.isObject())
	{
		NamedValueSet lData = lostData.getDynamicObject()->getProperties();
		for (auto& ld : lData)
		{
			lostControllables.set(ld.name.toString(), (TransitionMode)(int)ld.value);
		}
	}
}

void Preset::controllableControlAddressChanged(Controllable* c)
{
	BaseItem::controllableControlAddressChanged(c);
	bool isAttachedToRoot = ControllableUtil::findParentAs<Engine>(c) != nullptr;
	if (isAttachedToRoot && dataMap.contains(c)) updateControllableAddress(c);
	else
	{
		//got detached, meaning it will be surely removed
		if (dataMap.contains(c))
		{
			dataMap.remove(c);
			c->removeControllableListener(this);
			lostControllables.set(controllableGhostAddressMap[c], transitionMap[c]);
			controllableGhostAddressMap.remove(c);
		}
	}
}

void Preset::childStructureChanged(ControllableContainer* cc)
{
	if (cc != Engine::mainEngine) BaseItem::childStructureChanged(cc);

	if (Engine::mainEngine->isLoadingFile) return;
	recoverLostControllables();
}

void Preset::endLoadFile()
{
	Engine::mainEngine->removeEngineListener(this);
	recoverLostControllables();
}

InspectableEditor* Preset::getEditorInternal(bool isRoot, Array<Inspectable*> controllables)
{
	return new PresetEditor(this, isRoot);
}
