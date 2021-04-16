/*
  ==============================================================================

    AudioHelpers.h
    Created: 16 Apr 2021 2:55:07pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once
#include "JuceHeader.h"

class DecibelFloatParameter :
    public FloatParameter
{
public:
    DecibelFloatParameter(const String& niceName, const String& description);
    ~DecibelFloatParameter();

    float gain;
    float decibels;

    void setGain(float gain);
    void setValueInternal(var& val) override;

    float valueToDecibels(float val);
    float decibelsToValue(float decibels);

    ControllableUI* createDefaultUI() override;
};

class VolumeControl :
    public ControllableContainer
{
public:
    VolumeControl(const String& name, bool hasRMS = false);
    virtual ~VolumeControl();

    float prevGain;
    DecibelFloatParameter* gain;
    DecibelFloatParameter* rms;
    BoolParameter* active;

    int rmsSampleCount;
    float rmsMax;

    virtual float getGain();
    virtual void resetGainAndActive();

    virtual void applyGain(AudioSampleBuffer& buffer);
    virtual void applyGain(int channel, AudioSampleBuffer& buffer);

    virtual void updateRMS(AudioSampleBuffer& buffer, int channel = -1, int startSample = 0, int numSamples = -1);
};