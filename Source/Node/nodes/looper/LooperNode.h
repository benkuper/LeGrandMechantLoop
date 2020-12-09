/*
  ==============================================================================

    LooperNode.h
    Created: 15 Nov 2020 8:43:03am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "../../Node.h"
#include "Transport/Transport.h"

class LooperTrack;

class LooperNode :
        public Node,
        public Transport::TransportListener
{
public:
    enum LooperType { AUDIO, MIDI };

    LooperNode(StringRef name, var params, LooperType looperType);
    virtual ~LooperNode();

    LooperType looperType;

    enum MonitorMode { OFF, ALWAYS, RECORDING_ONLY, ARMED_TRACK };
    EnumParameter* monitorMode;
    
    IntParameter* currentTrackIndex;
    IntParameter* numTracks;

    EnumParameter* quantization;
    EnumParameter* freeFillMode;

    Trigger* recTrigger;
    Trigger* clearCurrentTrigger;
    Trigger* playAllTrigger;
    Trigger* stopAllTrigger;
    Trigger* clearAllTrigger;

    LooperTrack* currentTrack;
    ControllableContainer tracksCC;
    
    IntParameter* fadeTimeMS;

    virtual void initInternal() override;

    virtual void updateLooperTracks();
    virtual LooperTrack* createLooperTrack(int index) { return nullptr; }

    virtual void setCurrentTrack(LooperTrack* t);

    virtual void onContainerTriggerTriggered(Trigger* t) override;
    virtual void onContainerParameterChangedInternal(Parameter* p) override;
    virtual void onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c) override;

    virtual void beatChanged(bool isNewBar) override;
    virtual void playStateChanged(bool isPlaying) override;

    //helpers
    virtual bool hasContent();
    virtual bool isOneTrackRecording(bool includeWillRecord = false);
    LooperTrack* getTrackForIndex(int index);

    Transport::Quantization getQuantization();
    Transport::Quantization getFreeFillMode();

    BaseNodeViewUI* createViewUI() override;
};
