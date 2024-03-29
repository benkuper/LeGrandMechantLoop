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
    std::unique_ptr<RingBuffer<float>> retroRingBuffer;

    enum TrackOutputMode { MIXED_ONLY, SEPARATE_ONLY, ALL };
    EnumParameter* trackOutputMode;


    void initInternal() override;

    virtual void updateOutTracks(bool updateConfig = true);
    virtual LooperTrack * createLooperTrack(int index) override;
    virtual void updateRingBuffer();
    virtual void updateRetroRingBuffer();
    virtual int getFadeNumSamples(); //for ring buffer fade

    virtual void audioSetupChanged() override;

    virtual void prepareToPlay(double sampleRate, int maximumExpectedSamplePerBlock) override;

    void onContainerParameterChangedInternal(Parameter* p) override;
    void onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c) override;

    virtual void bpmChanged() override;
    virtual void playStateChanged(bool isPlaying, bool forceRestart) override;

    virtual void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

    String getTypeString() const override { return getTypeStringStatic(); }
    static String getTypeStringStatic() { return "Audio Looper"; }
};