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
}

MappingManager::~MappingManager()
{
}