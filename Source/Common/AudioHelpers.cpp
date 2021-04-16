/*
  ==============================================================================

    AudioHelpers.cpp
    Created: 16 Apr 2021 2:55:07pm
    Author:  bkupe

  ==============================================================================
*/

#include "AudioHelpers.h"
#include "AudioUIHelpers.h"

DecibelFloatParameter::DecibelFloatParameter(const String& niceName, const String& description, float initValue) :
	FloatParameter(niceName, description, initValue, 0, 1)
{
	decibels = valueToDecibels(value);
	gain = Decibels::decibelsToGain(valueToDecibels(value), -100.f);
}

DecibelFloatParameter::~DecibelFloatParameter()
{
}

void DecibelFloatParameter::setGain(float newGain)
{
	setValue(decibelsToValue(Decibels::gainToDecibels(newGain, -100.0f)));
}

void DecibelFloatParameter::setValueInternal(var& val)
{
	FloatParameter::setValueInternal(val);
	decibels = valueToDecibels(value);
	gain = Decibels::decibelsToGain(valueToDecibels(value), -100.f);
}

float DecibelFloatParameter::valueToDecibels(float val)
{
	float lowFunc = jmin(jmap<float>(val, 0, .14f, -100, -42), -42.f);
	float highFunc = jmax(jmap<float>(val, .4f, 1, -18, 6), -18.f);

	if (val <= .14f) return lowFunc;
	else if (val >= .4f) return highFunc;

	float w = jlimit(0.f, 1.f, jmap(val, .14f, .4f, 0.f, 1.f));
	return lowFunc * (1 - w) + highFunc * w;
}

float DecibelFloatParameter::decibelsToValue(float decibels)
{
	float lowFunc = jmin(jmap<float>(decibels, -100, -42, 0, .14f), .14f);
	float highFunc = jmax(jmap<float>(decibels, -18, 6, .4f, 1), .4f);

	if (decibels <= -42) return lowFunc;
	else if (decibels >= -18) return highFunc;

	float w = jlimit(0.f, 1.f, jmap(decibels, -42.0f, -18.0f, 0.f, 1.f));
	return lowFunc * (1 - w) + highFunc * w;
}

ControllableUI* DecibelFloatParameter::createDefaultUI()
{
	return new DecibelSliderUI(this);
}

VolumeControl::VolumeControl(const String& name, bool hasRMS) :
	ControllableContainer(name),
	prevGain(1),
	rms(nullptr),
	rmsSampleCount(0),
	rmsMax(0)
{
	gain = new DecibelFloatParameter("Gain", "Gain for this");
	addParameter(gain);
	active = addBoolParameter("Active", "Fast way to mute this", true);

	if (hasRMS)
	{
		rms = new DecibelFloatParameter("RMS", "RMS for this");
		addParameter(rms);
		rms->setControllableFeedbackOnly(true);
		rms->hideInRemoteControl = true; //hide by default
		rms->defaultHideInRemoteControl = true; //hide by default
	}
}

VolumeControl::~VolumeControl()
{
}

float VolumeControl::getGain()
{
	if (active == nullptr || gain == nullptr) return 0;
	return active->boolValue() ? gain->floatValue() : 0;
}

void VolumeControl::resetGainAndActive()
{
	gain->resetValue();
	active->resetValue();
}

void VolumeControl::applyGain(AudioSampleBuffer& buffer)
{
	float g = getGain();
	buffer.applyGainRamp(0, buffer.getNumSamples(), prevGain, g);
	prevGain = g;

	updateRMS(buffer);
}

void VolumeControl::applyGain(int channel, AudioSampleBuffer& buffer)
{
	float g = getGain();
	buffer.applyGainRamp(channel, 0, buffer.getNumSamples(), prevGain, g);
	prevGain = g;

	updateRMS(buffer, channel);
}

void VolumeControl::updateRMS(AudioSampleBuffer& buffer, int channel, int startSample, int numSamples)
{
	if (numSamples == -1) numSamples = buffer.getNumSamples();

	if (rms != nullptr)
	{
		if (channel >= 0) rmsMax = jmax(buffer.getRMSLevel(channel, startSample, numSamples), rmsMax);
		else
		{
			for (int i = 0; i < buffer.getNumChannels(); i++) rmsMax = jmax(buffer.getRMSLevel(i, startSample, numSamples), rmsMax);
		}

		rmsSampleCount += buffer.getNumSamples();
		if (rmsSampleCount > 4000) //~10fps @44100Hz
		{
			float rmsGainVal = rms->gain + (rmsMax - rms->gain) * (rmsMax > rms->gain ? .8f : .2f);
			rms->setGain(rmsMax);
			
			rmsSampleCount = 0;
			rmsMax = 0;
		}
	}
}