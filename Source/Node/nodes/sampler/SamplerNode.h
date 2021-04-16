/*
  ==============================================================================

    SamplerNode.h
    Created: 14 Apr 2021 8:40:43am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "../../Node.h"
#include "Common/MIDI/MIDIDeviceParameter.h"
#include "Common/RingBuffer.h"

class SamplerNode :
    public Node,
    public MIDIInputDevice::MIDIInputListener,
    public MidiKeyboardStateListener
{
public:
    SamplerNode(var params);
    virtual ~SamplerNode();

    enum PlayMode { HIT, LOOP, HOLD};

    IntParameter* numChannels;
    BoolParameter* showKeyboard;
    EnumParameter* playMode;
    BoolParameter* monitor;

    BoolParameter* clearMode;
    Trigger* clearLastRecordedTrigger;
    Trigger* clearAllNotesTrigger;

    FloatParameter* attack;
    FloatParameter* decay;
    DecibelFloatParameter* sustain;
    FloatParameter* release;

    BoolParameter * isRecording;
    int recordingNote;
    int lastRecordedNote;

    struct SamplerNote
    {
    public:
        bool isRecording = false;
        int playingSample = 0;
        float velocity = 0;
        ADSR adsr;
        AudioSampleBuffer buffer;
        bool hasContent() { return buffer.getNumSamples() > 0; }
    };

    OwnedArray<SamplerNote> samplerNotes;

    std::unique_ptr<RingBuffer<float>> ringBuffer;
    AudioSampleBuffer preRecBuffer;
    int recordedSamples;

    MIDIDeviceParameter* midiParam;
    IntParameter* fadeTimeMS;

    MIDIInputDevice* currentDevice;
    MidiMessageCollector midiCollector;
    MidiKeyboardState keyboardState;

    
    void clearNote(int note);
    void clearAllNotes();

    void updateBuffers();
    void updateRingBuffer();

    void startRecording(int note);
    void stopRecording();

    void exportSampleSet(File folder);
    void importSampleSet(File folder);

    void setMIDIDevice(MIDIInputDevice* d);

    void onContainerTriggerTriggered(Trigger* t) override;
    void onContainerParameterChangedInternal(Parameter* p) override;
    void controllableStateChanged(Controllable* c) override;

    void midiMessageReceived(const MidiMessage& m) override;

    virtual void handleNoteOn(MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity) override;
    virtual void handleNoteOff(MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity) override;

    virtual int getFadeNumSamples(); //for ring buffer fade


    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;
    //void processBlockBypassed(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;


    String getTypeString() const override { return getTypeStringStatic(); }
    static const String getTypeStringStatic() { return "Sampler"; }

    BaseNodeViewUI* createViewUI() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SamplerNode)
};