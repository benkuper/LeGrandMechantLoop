/*
  ==============================================================================

	InterfaceManager.cpp
	Created: 15 Nov 2020 8:43:43am
	Author:  bkupe

  ==============================================================================
*/

#include "Interface/InterfaceIncludes.h"

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

void InterfaceManager::addItemInternal(Interface* i, var data)
{
	BaseManager::addItemInternal(i, data);
	if (MIDIInterface* mi = dynamic_cast<MIDIInterface*>(i))
	{
		mi->addMIDIInterfaceListener(this);
	}
}

void InterfaceManager::removeItemInternal(Interface* i)
{
	BaseManager::removeItemInternal(i);
	if (MIDIInterface* mi = dynamic_cast<MIDIInterface*>(i))
	{
		mi->removeMIDIInterfaceListener(this);
	}
}

void InterfaceManager::midiMessageReceived(MIDIInterface* i, const MidiMessage& msg)
{
	interfaceManagerListeners.call(&InterfaceManagerListener::managerMidiMessageReceived, i, msg);
}
