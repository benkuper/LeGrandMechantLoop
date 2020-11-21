/*
  ==============================================================================

    LooperNode.h
    Created: 15 Nov 2020 8:43:03am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "../../NodeAudioProcessor.h"

class LooperTrack;

class LooperProcessor :
        public GenericNodeAudioProcessor
{
public:
    LooperProcessor(Node * node);
    ~LooperProcessor() {}

    IntParameter* numTracks;
    IntParameter* numChannelsPerTrack;
    
    ControllableContainer tracksCC;

    enum TrackOutputMode { MIXED_ONLY, SEPARATE_ONLY, ALL };
    EnumParameter * trackOutputMode;

    enum MonitorMode { OFF, ALWAYS, RECORDING_ONLY, ARMED_TRACK };
    EnumParameter* monitorMode;

    void updateOutTracks();
    void updateLooperTracks();

    void onContainerParameterChanged(Parameter* p) override;

    void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;
    static String getTypeStringStatic() { return "Looper"; }
  
    NodeViewUI* createNodeViewUI() override;
};