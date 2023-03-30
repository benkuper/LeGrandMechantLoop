/*
  ==============================================================================

    InterfaceManager.h
    Created: 15 Nov 2020 8:43:43am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

class InterfaceManager :
    public BaseManager<Interface>,
    public MIDIInterface::MIDIInterfaceListener
{
public:
    juce_DeclareSingleton(InterfaceManager, true);
    InterfaceManager();
    ~InterfaceManager();

    Factory<Interface> factory;

    void addItemInternal(Interface* i, var data) override;
    void removeItemInternal(Interface* i) override;
    
    void midiMessageReceived(MIDIInterface*, const MidiMessage& msg) override;


    class InterfaceManagerListener
    {
    public:
        virtual ~InterfaceManagerListener() {}
        virtual void managerMidiMessageReceived(MIDIInterface*, const MidiMessage& msg) {}
    };

    ListenerList<InterfaceManagerListener> interfaceManagerListeners;
    void addInterfaceManagerListener(InterfaceManagerListener* newListener) { interfaceManagerListeners.add(newListener); }
    void removeInterfaceManagerListener(InterfaceManagerListener* listener) { interfaceManagerListeners.remove(listener); }
};