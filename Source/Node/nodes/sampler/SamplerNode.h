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

class SamplerNode :
    public Node,
    public MIDIInputDevice::MIDIInputListener
{
public:
    SamplerNode(var params);
    virtual ~SamplerNode();

    enum PlayMode { HIT, LOOP, REVERSE, REVERSE_LOOP };

    BoolParameter* showKeyboard;
    EnumParameter* playMode;

    FloatParameter* attack;
    FloatParameter* decay;
    FloatParameter* sustain;
    FloatParameter* release;

    Array<float> notesGains;
    Array<bool> notesActive; //to track animation

    MIDIDeviceParameter* midiParam;

    MIDIInputDevice* currentDevice;
    std::unique_ptr<AudioPluginInstance> vst;
    MidiMessageCollector midiCollector;
    MidiKeyboardState keyboardState;
    
    AudioSampleBuffer notesBuffers;

    void exportSampleSet(File folder);
    void importSampleSet(File folder);

    void setMIDIDevice(MIDIInputDevice* d);

    void onContainerParameterChangedInternal(Parameter* p) override;
    void controllableStateChanged(Controllable* c) override;

    void midiMessageReceived(const MidiMessage& m) override;

        
    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;
    //void processBlockBypassed(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

    String getTypeString() const override { return getTypeStringStatic(); }
    static const String getTypeStringStatic() { return "Sampler"; }

    BaseNodeViewUI* createViewUI() override;
};