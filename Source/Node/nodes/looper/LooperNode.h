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

class LooperTrack;

class LooperProcessor :
        public GenericNodeAudioProcessor,
        public Transport::TransportListener
{
public:
    LooperProcessor(Node * node);
    ~LooperProcessor();

    IntParameter* numTracks;
    IntParameter* numChannelsPerTrack;
    
    ControllableContainer tracksCC;

    enum TrackOutputMode { MIXED_ONLY, SEPARATE_ONLY, ALL };
    EnumParameter * trackOutputMode;

    enum MonitorMode { OFF, ALWAYS, RECORDING_ONLY, ARMED_TRACK };
    EnumParameter* monitorMode;

    void updateOutTracks();
    void updateLooperTracks();

    void trackStateChanged(LooperTrack * t);

    void onContainerParameterChanged(Parameter* p) override;
    void onControllableFeedbackUpdate(ControllableContainer * cc, Controllable* c) override;

    virtual void beatChanged(bool isNewBar) override;
    virtual void playStateChanged(bool isPlaying) override;


    void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

    //helpers
    bool isOneTrackRecording(bool includeWillRecord = false);

    static String getTypeStringStatic() { return "Looper"; }
    NodeViewUI* createNodeViewUI() override;
};