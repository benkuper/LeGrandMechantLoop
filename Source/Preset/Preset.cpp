/*
  ==============================================================================

	Preset.cpp
	Created: 17 Apr 2021 12:08:52pm
	Author:  bkupe

  ==============================================================================
*/

#include "Preset.h"
#include "PresetManager.h"

Preset::Preset(var params) :
	BaseItem("Preset", false)
{
	subPresets.reset(new PresetManager());
	addChildControllableContainer(subPresets.get());
	subPresets->addBaseManagerListener(this);

	Random r;
	color = addColorParameter("Color","Color for UI convenience", Colour::fromHSV(r.nextFloat(), .3f,.5f,1));
}

Preset::~Preset()
{
}

var Preset::getPresetData()
{
	var data(new DynamicObject());
	Array<Preset*> presets = getPresetChain();

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

var Preset::getPresetDataForControllable(Controllable* c)
{
	String add = c->getControlAddress();
	Array<Preset*> presets = getPresetChain();
	for (auto& p : presets) if (p->addressMap.contains(add)) return p->addressMap[add];
	return var();
}

void Preset::save(ControllableContainer* container, bool recursive)
{
	Array<WeakReference<Controllable>> cList = container->getAllControllables(recursive);
	for (auto& c : cList) save(c);
}

void Preset::save(Controllable* controllable)
{
	if (controllable != nullptr)
	{
		dataMap.set(controllable, controllable->getJSONData());
		if (!isMain()) overridenControllables.add(controllable->getControlAddress());
	}
	else
	{
		Array<WeakReference<Controllable>> controllables = Engine::mainEngine->getAllControllables(true);

		dataMap.clear();
		addressMap.clear();

		bool main = isMain();
		for (auto& c : controllables)
		{
			var d = c->getJSONData();
			String add = c->getControlAddress();
			if (!c->shouldBeSaved()) continue;
			if (main || overridenControllables.contains(add))
			{
				dataMap.set(c, d);
				addressMap.set(add, d);
			}
		}
	}
}

void Preset::load(ControllableContainer* container, bool recursive)
{
	Array<WeakReference<Controllable>> cList = container->getAllControllables(recursive);
	for (auto& c : cList) load(c);
}

void Preset::load(Controllable* controllable)
{
	if (controllable != nullptr)
	{
		var data = getPresetDataForControllable(controllable);
		controllable->loadJSONData(data);
	}
	else
	{
		var data = getPresetData();
		NamedValueSet props = data.getDynamicObject()->getProperties();
		for (auto& p : props)
		{
			if (Controllable* c = Engine::mainEngine->getControllableForAddress(p.name.toString()))
			{
				c->loadJSONData(p.value);
			}
		}
	}
}


bool Preset::isMain()
{
	return parentContainer == RootPresetManager::getInstance();
}

void Preset::itemAdded(Preset* p)
{
	if(!isCurrentlyLoadingData) p->color->setColor(color->getColor().brighter(.3f));
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
