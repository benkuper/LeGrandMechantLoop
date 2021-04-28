/*
  ==============================================================================

    AudioLooperNode.h
    Created: 1 Dec 2020 11:16:14pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

class AudioLooperNode :
    public LooperNode,
    public AudioManager::AudioManagerListener
{
public:
    AudioLooperNode(var params = var());
    ~AudioLooperNode();

    IntParameter* numChannelsPerTrack;
    std::unique_ptr<RingBuffer<float>> ringBuffer;

    enum TrackOutputMode { MIXED_ONLY, SEPARATE_ONLY, ALL };
    EnumParameter* trackOutputMode;

    void initInternal() override;

    virtual void updateOutTracks();
    virtual LooperTrack * createLooperTrack(int index) override;
    virtual void updateRingBuffer();
    virtual int getFadeNumSamples(); //for ring buffer fade

    virtual void audioSetupChanged() override;

    void onContainerParameterChangedInternal(Parameter* p) override;
    void onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c) override;

    virtual void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

    String getTypeString() const override { return getTypeStringStatic(); }
    static String getTypeStringStatic() { return "Audio Looper"; }
};