/*
  ==============================================================================

    AudioLooperNode.h
    Created: 1 Dec 2020 11:16:14pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "LooperNode.h"
class AudioLooperTrack;

class AudioLooperProcessor :
    public LooperProcessor,
    public AudioManager::AudioManagerListener
{
public:
    AudioLooperProcessor(Node * n);
    ~AudioLooperProcessor();

    IntParameter* numChannelsPerTrack;
    std::unique_ptr<RingBuffer<float>> ringBuffer;

    enum TrackOutputMode { MIXED_ONLY, SEPARATE_ONLY, ALL };
    EnumParameter* trackOutputMode;

    virtual void updateOutTracks();
    virtual void updateLooperTracks();
    virtual void updateRingBuffer();

    virtual void audioSetupChanged() override;

    void onContainerParameterChanged(Parameter* p) override;

    virtual void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

    static String getTypeStringStatic() { return "Audio Looper"; }
};