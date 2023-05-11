/*
  ==============================================================================

    AudioHelpers.h
    Created: 16 Apr 2021 2:55:07pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

class DecibelsHelpers
{
public:
    static Point<float> start;
    static Point<float> mid1;
    static Point<float> mid2;
    static Point<float> end; 
    
    dsp::LookupTableTransform<float> valToDecLut;

    static void init();
    static float valueToDecibels(float val);
    static float decibelsToValue(float decibels);
    static float valueToGain(float value);
    static float gainToValue(float gain);
};

class DecibelFloatParameter :
    public FloatParameter
{
public:
    DecibelFloatParameter(const String& niceName, const String& description, float initValue = .85f);
    ~DecibelFloatParameter();

    float gain;
    float decibels;

    void setGain(float gain);
    void setValueInternal(var& val) override;

    ControllableUI* createDefaultUI(Array<Controllable*> controllables = {}) override;
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
    BoolParameter* computeRMS;

    int rmsSampleCount;
    float rmsMax;

    virtual float getGain();
    virtual void resetGainAndActive();

    virtual void applyGain(AudioSampleBuffer& buffer);
    virtual void applyGain(int channel, AudioSampleBuffer& buffer);

    virtual void updateRMS(AudioSampleBuffer& buffer, int channel = -1, int startSample = 0, int numSamples = -1);
};