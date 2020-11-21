/*
  ==============================================================================

    Transport.h
    Created: 15 Nov 2020 8:53:43am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"

class Transport :
    public ControllableContainer,
    public AudioIODeviceCallback
   // public AudioPlayHead
{
public:
    juce_DeclareSingleton(Transport, true);

    Transport();
    ~Transport();

    enum Quantization { BAR, BEAT, FREE };
    EnumParameter* quantization;

    FloatParameter * bpm;
    IntParameter* beatsPerBar; //first part of time signature
    IntParameter* beatUnit; //2nd part of time signature

    BoolParameter* isPlayingParam;
    FloatParameter* currentTime;
    IntParameter* curBar;
    IntParameter* curBeat;
    FloatParameter* barProgression;
    FloatParameter* beatProgression;

    Trigger* playTrigger;
    Trigger* togglePlayTrigger;
    Trigger* pauseTrigger;
    Trigger* stopTrigger;

    int sampleRate;
    int numSamplesPerBeat;
    int blockSize;

    int timeInSamples;
    double curTimeHiRes;
    bool isSettingTempo;
    
    double timeAtStart;

    void clear(); // override here to avoid deleting parameters

    //Controls
    void play(bool startTempoSet = false);
    void pause();
    void stop();

    void finishSetTempo(bool startPlaying = true);
    
    void setCurrentTimeFromSamples(int samples);
    void setCurrentTime(double time);
    void gotoBar(int bar);
    void gotoBeat(int beat, int bar = -1);

    //Events
    void onContainerTriggerTriggered(Trigger* t) override;
    void onContainerParameterChanged(Parameter* p) override;


    //Helpers
    double getBarLength() const;
    double getBeatLength() const;
    double getBarNumSamples() const;
    double getBeatNumSamples() const;
    double getTimeToNextBar() const;
    double getTimeToNextBeat() const;

    double getTimeForBar(int bar = -1) const;
    double getTimeForBeat(int beat = -1, int bar = -1, bool relative = true) const;
    int getBarForTime(double time) const;
    int getBeatForTime(double time) const;
    int getBarForSamples(int samples) const;
    int getBeatForSamples(int samples, bool relative = true) const;

    double getBeatTimeInSamples(int beat = -1) const;


    // Inherited via AudioIODeviceCallback
    virtual void audioDeviceIOCallback(const float** inputChannelData, int numInputChannels, float** outputChannelData, int numOutputChannels, int numSamples) override;
    virtual void audioDeviceAboutToStart(AudioIODevice* device) override;
    virtual void audioDeviceStopped() override;

    // Inherited via AudioPlayHead
    //virtual bool getCurrentPosition(CurrentPositionInfo& result) override;



   
    //DECLARE_ASYNC_EVENT(Transport, Transport, transport, ENUM_LIST(TIME_SIGNATURE_CHANGED, BPM_CHANGED))
};