/*
  ==============================================================================

    InterfaceManager.cpp
    Created: 15 Nov 2020 8:43:43am
    Author:  bkupe

  ==============================================================================
*/

juce_ImplementSingleton(InterfaceManager)

InterfaceManager::InterfaceManager() :
    BaseManager("Interfaces")
{
    factory.defs.add(Factory<Interface>::Definition::createDef<MIDIInterface>("", "MIDI"));
    factory.defs.add(Factory<Interface>::Definition::createDef<OSCInterface>("", "OSC"));
    managerFactory = &factory;
}

InterfaceManager::~InterfaceManager()
{
}
