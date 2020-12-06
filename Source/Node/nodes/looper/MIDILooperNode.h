/*
  ==============================================================================

    MIDILooperNode.h
    Created: 1 Dec 2020 11:15:31pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "LooperNode.h"
#include "Common/MIDI/MIDIDeviceParameter.h"
class MIDILooperTrack;

class MIDILooperNode :
    public LooperNode,
    public MIDIInputDevice::MIDIInputListener
{
public:
    MIDILooperNode(var params= var());
    ~MIDILooperNode();

    MIDIDeviceParameter* midiParam;
    MIDIInputDevice* currentDevice;

    MidiMessageCollector collector;
    MidiMessageCollector cleanupCollector;

    struct NoteInfo { 
        int channel;  
        int noteNumber; 
        inline bool operator == (const NoteInfo& other) const { return channel == other.channel && noteNumber == other.noteNumber; }
    };
    Array<NoteInfo> currentNoteOns; //keeping track of onNotes to cleanup good

    void setMIDIDevice(MIDIInputDevice* d);

    virtual LooperTrack* createLooperTrack(int index) override;

    void midiMessageReceived(const MidiMessage& m) override;
    void onContainerParameterChangedInternal(Parameter* p) override;

    void onControllableFeedbackUpdateInternal(ControllableContainer * cc, Controllable *c) override;

    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;

    virtual void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;
    
    String getTypeString() const override { return getTypeStringStatic(); }
    static String getTypeStringStatic() { return "MIDI Looper"; }
};