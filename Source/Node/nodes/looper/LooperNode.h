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
    
    LooperTrack* currentTrack;
    ControllableContainer tracksCC;

    ControllableContainer trackParamsCC;
    IntParameter* numTracks;
    IntParameter* currentTrackIndex;
    
    ControllableContainer recordCC;
    BoolParameter* isRecording;
    EnumParameter* quantization;
    EnumParameter* freeFillMode;
    IntParameter* fadeTimeMS;
    enum DoubleRecMode { NOTHING, AUTO_STOP_BAR, AUTO_STOP_BEAT };
    EnumParameter* doubleRecMode;
    IntParameter* doubleRecVal;
    enum TempMuteMode { NEXT_BAR, NEXT_BEAT };
    EnumParameter* tmpMuteMode;
    FloatParameter* firstRecVolumeThreshold;


    ControllableContainer controlsCC;
    Trigger* recTrigger;
    Trigger* clearCurrentTrigger;
    Trigger* playAllTrigger;
    Trigger* stopAllTrigger;
    Trigger* clearAllTrigger;
    Trigger* tmpMuteAllTrigger;

    //tmp mute
    Array<LooperTrack *> tmpMuteTracks;

    virtual void initInternal() override;

    virtual void updateLooperTracks();
    virtual LooperTrack* createLooperTrack(int index) { return nullptr; }

    virtual void setCurrentTrack(LooperTrack* t);

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
