/*
  ==============================================================================

    MIDILooperNode.h
    Created: 1 Dec 2020 11:15:31pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

class MIDILooperNode :
    public LooperNode
{
public:
    MIDILooperNode(var params= var());
    ~MIDILooperNode();

    MidiMessageCollector cleanupCollector;

    struct NoteInfo { 
        int channel;  
        int noteNumber; 
        inline bool operator == (const NoteInfo& other) const { return channel == other.channel && noteNumber == other.noteNumber; }
    };
    Array<NoteInfo> currentNoteOns; //keeping track of onNotes to cleanup good

    virtual LooperTrack* createLooperTrack(int index) override;

   // virtual  void receiveMIDIFromInput(Node* n, MidiBuffer& inputBuffer) override;
    virtual void midiMessageReceived(MIDIInterface* i, const MidiMessage& m) override;

    void onControllableFeedbackUpdateInternal(ControllableContainer * cc, Controllable *c) override;

    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;

    virtual void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;
    
    String getTypeString() const override { return getTypeStringStatic(); }
    static String getTypeStringStatic() { return "MIDI Looper"; }
};