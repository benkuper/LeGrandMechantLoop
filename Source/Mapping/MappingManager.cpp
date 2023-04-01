/*
  ==============================================================================

	MappingManager.cpp
	Created: 3 May 2021 10:23:44am
	Author:  bkupe

  ==============================================================================
*/

#include "MappingManager.h"

juce_ImplementSingleton(MappingManager)

MappingManager::MappingManager() :
	BaseManager("Mappings")
{
	factory.defs.add(Factory<Mapping>::Definition::createDef<MIDIMapping>("", "MIDI"));
	factory.defs.add(Factory<Mapping>::Definition::createDef<MacroMapping>("", "Macro"));
	factory.defs.add(Factory<Mapping>::Definition::createDef<GenericMapping>("", "Generic"));
	managerFactory = &factory;

	Engine::mainEngine->addEngineListener(this);
}

MappingManager::~MappingManager()
{
	if(Engine::mainEngine != nullptr) Engine::mainEngine->removeEngineListener(this);
}

void MappingManager::createMappingForControllable(Controllable* c, const String& type)
{
	Mapping* m = factory.create(type);
	if (m == nullptr) return;
	m->destParam->setValueFromTarget(c);
	addItem(m);
}

Mapping* MappingManager::getMappingForDestControllable(Controllable* c)
{
	for (auto& m : items) if (m->destParam->target == c) return m;
	return nullptr;
}

void MappingManager::endLoadFile()
{
	for(auto & m : items) m->sendFeedback();
}
