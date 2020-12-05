/*
  ==============================================================================

    LooperNode.h
    Created: 15 Nov 2020 8:43:03am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "../../NodeProcessor.h"
#include "Transport/Transport.h"

class LooperTrack;

class LooperProcessor :
        public GenericNodeProcessor,
        public Transport::TransportListener
{
public:
    enum LooperType { AUDIO, MIDI };

    LooperProcessor(Node * node, LooperType looperType);
    virtual ~LooperProcessor();

    LooperType looperType;

    IntParameter* numTracks;
    IntParameter* fadeTimeMS;

    EnumParameter* quantization;
    EnumParameter* freeFillMode;

    Trigger* recTrigger;
    Trigger* clearCurrentTrigger;

    Trigger* playAllTrigger;
    Trigger* stopAllTrigger;
    Trigger* clearAllTrigger;

    BoolParameter* autoNext;
    IntParameter* currentTrackIndex;

    LooperTrack* currentTrack;

    ControllableContainer tracksCC;

    enum MonitorMode { OFF, ALWAYS, RECORDING_ONLY, ARMED_TRACK };
    EnumParameter* monitorMode;

    virtual void updateLooperTracks();
    virtual LooperTrack* createLooperTrack(int index) { return nullptr; }

    virtual void setCurrentTrack(LooperTrack* t);

    virtual void onContainerTriggerTriggered(Trigger* t) override;
    virtual void onContainerParameterChanged(Parameter* p) override;
    virtual void onControllableFeedbackUpdate(ControllableContainer* cc, Controllable* c) override;

    virtual void beatChanged(bool isNewBar) override;
    virtual void playStateChanged(bool isPlaying) override;

    //helpers
    virtual bool hasContent();
    virtual bool isOneTrackRecording(bool includeWillRecord = false);
    LooperTrack* getTrackForIndex(int index);

    Transport::Quantization getQuantization();
    Transport::Quantization getFreeFillMode();

    static String getTypeStringStatic() { jassertfalse;  return "Looper"; }
    NodeViewUI* createNodeViewUI() override;
};