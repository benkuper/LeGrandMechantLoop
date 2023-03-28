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
	HashMap<WeakReference<Parameter>, var>::Iterator it(dataMap);
	while (it.next()) if (it.getKey() != nullptr && !it.getKey().wasObjectDeleted())
	{
		it.getKey()->removeControllableListener(this);
	}
}

var Preset::getPresetValues(bool includeParents)
{
	var data(new DynamicObject());
	Array<Preset*> presets;

	if (includeParents) presets = getPresetChain();
	else presets.add(this);

	for (auto& p : presets)
	{
		HashMap<String, var>::Iterator it(p->addressMap);
		while (it.next())
		{
			if (!data.hasProperty(it.getKey())) data.getDynamicObject()->setProperty(it.getKey(), it.getValue());
		}
	}

	return data;
}

var Preset::getPresetValueForParameter(Parameter* p, bool includeParents)
{
	Array<Preset*> presets;
	if (includeParents) presets = getPresetChain();
	else presets.add(this);

	String add = p->getControlAddress();
	for (auto& p : presets) if (p->addressMap.contains(add)) return p->addressMap[add];
	return var();
}

void Preset::saveContainer(ControllableContainer* container, bool recursive)
{
	Array<WeakReference<Parameter>> cList = container->getAllParameters(recursive);
	for (auto& c : cList) save(c);
}

void Preset::save(Parameter* parameter, bool saveAllPresettables, bool noCheck)
{
	if (parameter != nullptr)
	{
		if (!RootPresetManager::getInstance()->isParameterPresettable(parameter)) return;
		addParameterToDataMap(parameter);
	}
	else
	{
		if (!isCurrent->boolValue() && !noCheck)
		{
			AlertWindow::showOkCancelBox(AlertWindow::WarningIcon, "Not the current preset", "This is not the currently loaded preset. Do you want still want to save to preset", "Yes", "No", nullptr, ModalCallbackFunction::create([=](int result)
				{
					if (result) save(parameter, saveAllPresettables, true);
				}));

			return;
		}


		Array<WeakReference<Parameter>> params = Engine::mainEngine->getAllParameters(true);

		dataMap.clear();
		addressMap.clear();

		int numSaved = 0;
		for (auto& p : params)
		{
			if (!RootPresetManager::getInstance()->isParameterPresettable(p)) continue;
			var d = p->value;
			String add = p->getControlAddress();
			if (!p->shouldBeSaved()) continue;
			if (saveAllPresettables || overridenControllables.contains(add))
			{
				addParameterToDataMap(p);
				numSaved++;
			}
		}

		NLOG(niceName, "Saved " << numSaved << " values.");
	}
}

void Preset::load(ControllableContainer* container, bool recursive)
{
	if (container == nullptr)
	{
		load(recursive);
		return;
	}

	Array<WeakReference<Parameter>> pList = container->getAllParameters(recursive);
	for (auto& p : pList) load(p);
}

void Preset::load(Parameter* parameter, bool recursive)
{
	if (parameter == nullptr)
	{
		load(recursive);
		return;
	}

	if (!RootPresetManager::getInstance()->isParameterPresettable(parameter)) return;
	var value = getPresetValueForParameter(parameter, recursive);
	parameter->setValue(value);
}


void Preset::load(bool recursive)
{
	int numLoaded = 0;
	var data = getPresetValues(recursive);
	NamedValueSet props = data.getDynamicObject()->getProperties();
	for (auto& p : props)
	{
		if (Parameter* tp = dynamic_cast<Parameter*>(Engine::mainEngine->getControllableForAddress(p.name.toString())))
		{
			if (!RootPresetManager::getInstance()->isParameterPresettable(tp)) continue;
			tp->setValue(p.value);
			numLoaded++;
		}
	}
	NLOG(niceName, "Loaded " << numLoaded << " values.");
}


void Preset::addParameterToDataMap(Parameter* p, var forceValue)
{
	if (p == nullptr) return;

	var val = forceValue.isVoid() ? p->value : forceValue;

	String add = p->getControlAddress();

	dataMap.set(p, val);
	addressMap.set(add, val);
	paramGhostAddressMap.set(p, add);
	lostParamAddresses.removeAllInstancesOf(add);
	/*if (!isMain()) */overridenControllables.addIfNotAlreadyThere(add);

	p->addControllableListener(this);
	registerLinkedInspectable(p);
}

void Preset::updateParameterAddress(Parameter* p)
{
	if (p == nullptr) return;

	if (paramGhostAddressMap.contains(p))
	{
		String oldAdd = paramGhostAddressMap[p];
		var oldVal = addressMap[oldAdd];
		removeAddressFromDataMap(oldAdd);
		addParameterToDataMap(p, oldVal);
	}
}

void Preset::removeParameterFromDataMap(Parameter* p)
{
	if (p == nullptr) return;

	String add = p->getControlAddress();
	dataMap.remove(p);
	paramGhostAddressMap.remove(p);
	p->removeControllableListener(this);
	unregisterLinkedInspectable(p);
	addressMap.remove(add);
	lostParamAddresses.removeAllInstancesOf(add);
	/*if (!isMain())*/ overridenControllables.removeAllInstancesOf(add);
}

void Preset::removeAddressFromDataMap(String address)
{
	if (Parameter* p = dynamic_cast<Parameter*>(Engine::mainEngine->getControllableForAddress(address)))
	{
		removeParameterFromDataMap(p);
		return;
	}

	paramGhostAddressMap.removeValue(address);
	addressMap.remove(address);
	/* if (!isMain()) */ overridenControllables.removeAllInstancesOf(address);
}

bool Preset::isMain()
{
	return parentContainer == RootPresetManager::getInstance();
}

bool Preset::hasPresetParam(Parameter* p)
{
	return dataMap.contains(p);
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
			if (Parameter* tp = dynamic_cast<Parameter*>(Engine::mainEngine->getControllableForAddress(p.name.toString())))
			{
				addParameterToDataMap(tp, p.value);
			}
		}
	}
}

void Preset::controllableControlAddressChanged(Controllable* c)
{
	BaseItem::controllableControlAddressChanged(c);
	if (Parameter* p = dynamic_cast<Parameter*>(c))
	{
		bool isAttachedToRoot = ControllableUtil::findParentAs<Engine>(p) != nullptr;
		if (isAttachedToRoot && dataMap.contains(p)) updateParameterAddress(p);
		else
		{
			//got detached, meaning it will be surely removed
			if (dataMap.contains(p))
			{
				dataMap.remove(p);
				p->removeControllableListener(this);
				lostParamAddresses.addIfNotAlreadyThere(paramGhostAddressMap[p]);
				paramGhostAddressMap.remove(p);
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
			addParameterToDataMap(p, addressMap.contains(add) ? addressMap[add] : var());
		}
	}
}

Array<Preset*> Preset::getPresetChain()
{
	Array<Preset*> result;
	result.add(this);
	if (parentContainer != nullptr && parentContainer != RootPresetManager::getInstance())
	{
		Preset* p = dynamic_cast<Preset*>(parentContainer->parentContainer.get());
		if (p != nullptr) result.addArray(p->getPresetChain()); // preset > manager > preset, so a parent preset is 2 levels up a child preset
	}

	for (auto& c : linkedPresetsCC.controllables)
	{
		if (!c->enabled) continue;
		Preset* p = dynamic_cast<Preset*>(((TargetParameter*)c)->targetContainer.get());
		if (p == nullptr) continue;

		result.add(p);
	}

	return result;
}

InspectableEditor* Preset::getEditorInternal(bool isRoot, Array<Inspectable*> controllables)
{
	return new PresetEditor(this, isRoot);
}
