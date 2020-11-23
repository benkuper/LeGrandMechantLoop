/*
  ==============================================================================

    LooperNode.h
    Created: 15 Nov 2020 8:43:03am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "../../NodeAudioProcessor.h"
#include "Transport/Transport.h"
#include "Common/RingBuffer.h"
#include "Engine/AudioManager.h"

class LooperTrack;

class LooperProcessor :
        public GenericNodeAudioProcessor,
        public Transport::TransportListener,
        public AudioManager::AudioManagerListener
{
public:
    LooperProcessor(Node * node);
    ~LooperProcessor();

    IntParameter* numTracks;
    IntParameter* numChannelsPerTrack;
    IntParameter* fadeTimeMS;

    std::unique_ptr<RingBuffer<float>> ringBuffer;

    ControllableContainer tracksCC;

    enum TrackOutputMode { MIXED_ONLY, SEPARATE_ONLY, ALL };
    EnumParameter * trackOutputMode;

    enum MonitorMode { OFF, ALWAYS, RECORDING_ONLY, ARMED_TRACK };
    EnumParameter* monitorMode;

    void updateOutTracks();
    void updateLooperTracks();
    void updateRingBuffer();

    void onContainerParameterChanged(Parameter* p) override;

    virtual void beatChanged(bool isNewBar) override;
    virtual void playStateChanged(bool isPlaying) override;

    virtual void audioSetupChanged() override;
    
    void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

    //helpers
    bool isOneTrackRecording(bool includeWillRecord = false);
    int getFadeNumSamples(); //for ring buffer fade

    static String getTypeStringStatic() { return "Looper"; }
    NodeViewUI* createNodeViewUI() override;
};