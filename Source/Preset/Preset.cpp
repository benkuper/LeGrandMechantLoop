/*
  ==============================================================================

	Preset.cpp
	Created: 17 Apr 2021 12:08:52pm
	Author:  bkupe

  ==============================================================================
*/

#include "Preset/PresetIncludes.h"

Preset::Preset(var params) :
	BaseItem(getTypeString(), false),
	linkedPresetsCC("Linked Presets")
{
	itemDataType = "Preset";

	subPresets.reset(new PresetManager());
	addChildControllableContainer(subPresets.get());
	subPresets->addBaseManagerListener(this);

	Random r;
	isCurrent = addBoolParameter("Is Current", "If this is the currently loaded preset", false);
	isCurrent->setControllableFeedbackOnly(true);

	setHasCustomColor(true);
	itemColor->forceSaveValue = true;

	saveTrigger = addTrigger("Save", "Save the current state in this preset. If this is a sub-preset, this will only save overriden values");
	loadTrigger = addTrigger("Load", "Load the values in this preset. It this is a sub-preset, this will fetch all values up to its root preset");

	skipInPrevNext = addBoolParameter("Skip in Prev/Next", "Skip this preset in the next/previous preset actions", false);
	noParentOnNearbyLoad = addBoolParameter("No Parent on Nearby Load", "If checked, this will prevent from loading the parent preset when loading this preset from a nearby preset", false);

	transitionTime = addFloatParameter("Transition Time", "Time to transition.", 0, 0);
	transitionTime->defaultUI = FloatParameter::TIME;

	directTransitionMode = addEnumParameter("Direct Transition Mode", "For Boolean, String, Enum and other non transitionnable parameters.");
	directTransitionMode->addOption("Change at start", AT_START)->addOption("Change at end", AT_END);
	transition.addKey(0, 0);
	transition.addKey(1, 1);
	transition.editorCanBeCollapsed = true;
	transition.editorIsCollapsed = true;
	addChildControllableContainer(&transition);

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

var Preset::getPresetValues(bool includeParents, Array<Controllable*> ignoreList)
{
	var data(new DynamicObject());

	ignoreList.addArray(ignoredControllables);

	HashMap<WeakReference<Controllable>, var>::Iterator it(dataMap);
	while (it.next())
	{
		WeakReference<Controllable> c = it.getKey();
		if (c == nullptr || c.wasObjectDeleted()) continue;
		if (ignoreList.contains(c)) continue;

		String add = c->getControlAddress();
		if (!data.hasProperty(add)) data.getDynamicObject()->setProperty(add, it.getValue());
		ignoreList.add(c);
	}

	Array<Preset*> presetsToInclude;
	if (includeParents && parentContainer != nullptr && parentContainer != RootPresetManager::getInstance())
	{
		presetsToInclude.add((Preset*)parentContainer->parentContainer.get());

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



//var Preset::getPresetValueForParameter(Parameter* p, bool includeParents)
//{
//	String add = p->getControlAddress();
//	if (ignoredControllables.contains(p)) return var();
//
//	Array<Preset*> presets;
//	if (includeParents) presets = getPresetChain(p);
//	else presets.add(this);
//
//	for (auto& p : presets) if (p->addressMap.contains(add)) return p->addressMap[add];
//	return var();
//}

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

//void Preset::load(ControllableContainer* container, bool recursive)
//{
//	if (container == nullptr)
//	{
//		load(recursive);
//		return;
//	}
//
//	Array<WeakReference<Parameter>> pList = container->getAllParameters(recursive);
//	for (auto& p : pList) load(p);
//}
//
//void Preset::load(Controllable* controllable, bool recursive)
//{
//	if (controllable == nullptr)
//	{
//		load(recursive);
//		return;
//	}
//
//	if (!RootPresetManager::getInstance()->isControllablePresettable(controllable)) return;
//
//	if (controllable->type == Controllable::TRIGGER)
//	{
//		((Trigger*)controllable)->trigger();
//	}
//	else
//	{
//		Parameter* parameter = (Parameter*)controllable;
//		var value = getPresetValueForParameter(parameter, recursive);
//		if (!value.isVoid()) parameter->setValue(value);
//	}
//
//}


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
			else ((Parameter*)tc)->setValue(p.value);

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
	lostParamAddresses.removeAllInstancesOf(add);
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
	controllableGhostAddressMap.remove(c);
	c->removeControllableListener(this);
	unregisterLinkedInspectable(c);
	addressMap.remove(add);
	lostParamAddresses.removeAllInstancesOf(add);
	overridenControllables.removeAllInstancesOf(add);
}

void Preset::removeAddressFromDataMap(String address)
{
	if (Parameter* p = dynamic_cast<Parameter*>(Engine::mainEngine->getControllableForAddress(address)))
	{
		removeControllableFromDataMap(p);
		return;
	}

	controllableGhostAddressMap.removeValue(address);
	addressMap.remove(address);
	/* if (!isMain()) */ overridenControllables.removeAllInstancesOf(address);
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
	data.getDynamicObject()->setProperty("values", getPresetValues(false));

	var ignoreData;
	for (auto& c : ignoredControllables) ignoreData.append(c->getControlAddress());
	data.getDynamicObject()->setProperty("ignores", ignoreData);

	return data;
}

void Preset::loadJSONDataItemInternal(var data)
{
	subPresets->loadJSONData(data["subPresets"]);
	linkedPresetsCC.loadJSONData(data["linkedPresets"], true);

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
}

void Preset::controllableControlAddressChanged(Controllable* c)
{
	BaseItem::controllableControlAddressChanged(c);
	if (Parameter* p = dynamic_cast<Parameter*>(c))
	{
		bool isAttachedToRoot = ControllableUtil::findParentAs<Engine>(p) != nullptr;
		if (isAttachedToRoot && dataMap.contains(p)) updateControllableAddress(p);
		else
		{
			//got detached, meaning it will be surely removed
			if (dataMap.contains(p))
			{
				dataMap.remove(p);
				p->removeControllableListener(this);
				lostParamAddresses.addIfNotAlreadyThere(controllableGhostAddressMap[p]);
				controllableGhostAddressMap.remove(p);
			}
		}
	}

}

void Preset::childStructureChanged(ControllableContainer* cc)
{
	for (auto& add : lostParamAddresses)
	{
		if (Parameter* p = dynamic_cast<Parameter*>(Engine::mainEngine->getControllableForAddress(add)))
		{
			addControllableToDataMap(p, addressMap.contains(add) ? addressMap[add] : var());
		}
	}
}

//Array<Preset*> Preset::getPresetChain(Controllable* c)
//{
//	Array<Preset*> result;
//
//	if (c != nullptr && ignoredControllables.contains(c)) return result;
//
//	result.add(this);
//	if (parentContainer != nullptr && parentContainer != RootPresetManager::getInstance())
//	{
//		Preset* p = dynamic_cast<Preset*>(parentContainer->parentContainer.get());
//		if (p != nullptr) result.addArray(p->getPresetChain(c)); // preset > manager > preset, so a parent preset is 2 levels up a child preset
//	}
//
//	for (auto& c : linkedPresetsCC.controllables)
//	{
//		if (!c->enabled) continue;
//		Preset* p = dynamic_cast<Preset*>(((TargetParameter*)c)->targetContainer.get());
//		if (p == nullptr) continue;
//		result.add(p);
//	}
//
//	return result;
//}

//Array<Controllable*> Preset::getAllPresettableControllables(bool recursive, Array<String> ignoreList)
//{
//	ignoreList.addArray(ignoredControllables);
//
//	Array<Controllable*> result;
//	HashMap<WeakReference<Controllable>, var>::Iterator it(dataMap);
//	while (it.next())
//	{
//		WeakReference<Controllable> c = it.getKey();
//		if (c == nullptr || c.wasObjectDeleted()) continue;
//
//		String add = c->getControlAddress();
//		if (ignoreList.contains(add)) continue;
//
//		result.add(c);
//		ignoreList.add(add);
//	}
//
//	if (recursive)
//	{
//		if (parentContainer != nullptr && parentContainer != RootPresetManager::getInstance())
//		{
//			Preset* p = dynamic_cast<Preset*>(parentContainer->parentContainer.get());
//			if (p != nullptr) result.addArray(p->getAllPresettableControllables(true, ignoreList)); // preset > manager > preset, so a parent preset is 2 levels up a child preset
//		}
//	}
//	return result;
//}

InspectableEditor* Preset::getEditorInternal(bool isRoot, Array<Inspectable*> controllables)
{
	return new PresetEditor(this, isRoot);
}
