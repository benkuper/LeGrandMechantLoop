/*
  ==============================================================================

	Preset.cpp
	Created: 17 Apr 2021 12:08:52pm
	Author:  bkupe

  ==============================================================================
*/

Preset::Preset(var params) :
	BaseItem("Preset", false)
{
	itemDataType = "Preset";

	subPresets.reset(new PresetManager());
	addChildControllableContainer(subPresets.get());
	subPresets->addBaseManagerListener(this);

	Random r;
	color = addColorParameter("Color", "Color for UI convenience", Colour::fromHSV(r.nextFloat(), .3f, .5f, 1));
	color->forceSaveValue = true;
	saveTrigger = addTrigger("Save", "Save the current state in this preset. If this is a sub-preset, this will only save overriden values");
	loadTrigger = addTrigger("Load", "Load the values in this preset. It this is a sub-preset, this will fetch all values up to its root preset");
	isCurrent = addBoolParameter("Is Current", "If this is the currently loaded preset", false);
	isCurrent->setControllableFeedbackOnly(true);
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

var Preset::getPresetValueForParameter(Parameter* p)
{
	String add = p->getControlAddress();
	Array<Preset*> presets = getPresetChain();
	for (auto& p : presets) if (p->addressMap.contains(add)) return p->addressMap[add];
	return var();
}

void Preset::saveContainer(ControllableContainer* container, bool recursive)
{
	Array<WeakReference<Parameter>> cList = container->getAllParameters(recursive);
	for (auto& c : cList) save(c);
}

void Preset::save(Parameter * parameter, bool saveAllPresettables)
{
	if (parameter != nullptr)
	{
		if (!RootPresetManager::getInstance()->isParameterPresettable(parameter)) return;
		addParameterToDataMap(parameter);
	}
	else
	{
		if (!isCurrent->boolValue())
		{
			bool result = AlertWindow::showOkCancelBox(AlertWindow::WarningIcon, "Not the current preset", "This is not the currently loaded preset. Do you want still want to save to preset", "Yes", "No");
			if (!result) return;
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
	Array<WeakReference<Parameter>> pList = container->getAllParameters(recursive);
	for (auto& p : pList) load(p);
}

void Preset::load(Parameter* parameter)
{
	if (parameter != nullptr)
	{
		if (!RootPresetManager::getInstance()->isParameterPresettable(parameter)) return;
		var value = getPresetValueForParameter(parameter);
		parameter->setValue(value);
	}
	else
	{
		int numLoaded = 0;
		var data = getPresetValues();
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
	if (Parameter* p = dynamic_cast<Parameter *>(Engine::mainEngine->getControllableForAddress(address)))
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
	if(!isCurrentlyLoadingData) p->color->setColor(color->getColor().brighter(.3f));
}

var Preset::getJSONData()
{
	var data = BaseItem::getJSONData();
	data.getDynamicObject()->setProperty("subPresets", subPresets->getJSONData());
	data.getDynamicObject()->setProperty("values", getPresetValues(false));
	return data;
}

void Preset::loadJSONDataItemInternal(var data)
{
	subPresets->loadJSONData(data["subPresets"]);
	
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
		if (Parameter *p = dynamic_cast<Parameter *>(Engine::mainEngine->getControllableForAddress(add)))
		{
			addParameterToDataMap(p, addressMap.contains(add) ? addressMap[add] : var());
		}
	}
}

Array<Preset*> Preset::getPresetChain()
{
	Array<Preset*> result;
	result.add(this);
	ControllableContainer* pc = parentContainer;
	while (pc != RootPresetManager::getInstance())
	{
		if (Preset* p = dynamic_cast<Preset*>(pc)) result.add(p);
		pc = pc->parentContainer;
	}

	return result;
}

InspectableEditor* Preset::getEditorInternal(bool isRoot, Array<Inspectable*> controllables)
{
	return new PresetEditor(this, isRoot);
}
