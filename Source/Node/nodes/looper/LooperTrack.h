/*
  ==============================================================================

    LooperTrack.h
    Created: 21 Nov 2020 6:33:06pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"

class LooperTrack :
    public ControllableContainer
{
public:
    LooperTrack(int index, int numChannels);
    ~LooperTrack();

    enum TrackState { IDLE, WILL_RECORD, RECORDING, FINISH_RECORDING, PLAYING, WILL_STOP, STOPPED, WILL_PLAY, STATES_MAX };
    static String trackStateNames[STATES_MAX];

    EnumParameter* trackState;

    Trigger* playRecordTrigger;
    Trigger* stopTrigger;
    Trigger* clearTrigger;

    BoolParameter* active;
    FloatParameter* volume;
    FloatParameter* rms;

    int index;
    int numChannels;

    int curSample;
    AudioBuffer<float> buffer;

    double timeAtStateChange;

    void setNumChannels(int num);
    void updateAudioBuffer();

    void recordOrPlayNow();

    void stateChanged();

    void startRecording();
    void stopRecordAndPlay();
    void clearContent();

    void onContainerTriggerTriggered(Trigger *t) override;
    void onContainerParameterChanged(Parameter* p) override;

    void handleNewBeat();
    void handleNewBar();


    void processBlock(AudioBuffer<float>& inputBuffer);

    //Helpers
    bool isRecording() const;
    bool isPlaying() const;
};