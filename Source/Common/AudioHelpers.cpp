/*
  ==============================================================================

	AudioHelpers.cpp
	Created: 16 Apr 2021 2:55:07pm
	Author:  bkupe

  ==============================================================================
*/

#include "Common/CommonIncludes.h"

Point<float> DecibelsHelpers::start = Point<float>(-100, 0);
Point<float> DecibelsHelpers::mid1 = Point<float>(-42, .14f);
Point<float> DecibelsHelpers::mid2 = Point<float>(-18, .4f);
Point<float> DecibelsHelpers::end = Point<float>(6, 1);


DecibelFloatParameter::DecibelFloatParameter(const String& niceName, const String& description, float initValue) :
	FloatParameter(niceName, description, initValue, 0, 1)
{
	decibels = DecibelsHelpers::valueToDecibels(value);
	gain = DecibelsHelpers::valueToGain(value);
}

DecibelFloatParameter::~DecibelFloatParameter()
{
}

void DecibelFloatParameter::setGain(float newGain)
{
	setValue(DecibelsHelpers::gainToValue(newGain));
}

void DecibelFloatParameter::setValueInternal(var& val)
{
	FloatParameter::setValueInternal(val);
	decibels = DecibelsHelpers::valueToDecibels(value);
	gain = DecibelsHelpers::valueToGain(value);
}


ControllableUI* DecibelFloatParameter::createDefaultUI(Array<Controllable*> controllables)
{
	return new DecibelSliderUI(this);
}



void DecibelsHelpers::init()
{

}

float DecibelsHelpers::valueToDecibels(float val)
{
	//y is val and x is decibel
	if (val <= mid1.y) return jmap<float>(val, start.y, mid1.y, start.x, mid1.x);
	else if (val >= mid2.y) return jmap<float>(val, mid2.y, end.y, mid2.x, end.x);

	return jmap<float>(val, mid1.y, mid2.y, mid1.x, mid2.x);
}

float DecibelsHelpers::decibelsToValue(float decibels)
{
	//y is val and x is decibel
	float lowCurve = jmap<float>(decibels, start.x, mid1.x, start.y, mid1.y);
	float highCurve = jmap<float>(decibels, mid2.x, end.x, mid2.y, end.y);

	if (decibels <= mid1.x) return lowCurve;// jmap<float>(decibels, start.x, mid1.x, start.y, mid1.y);
	else if (decibels >= mid2.x) return highCurve;// jmap<float>(decibels, mid2.x, end.x, mid2.y, end.y);

	return jmap<float>(decibels, mid1.x, mid2.x, mid1.y, mid2.y);

	//float w = jmap(decibels, mid1.x, mid2.x, 0.0f, 1.0f);
	//float smoothness = 1;
	//float sw = 1 / exp(-smoothness * (w - .5f));
	//return lowCurve * (1 - sw) + highCurve * sw;

	//float valAtDecLow0 = jmap<float>(0, start.x, mid1.x, start.y, mid1.y);
	//float valAtDecHigh0 = jmap<float>(0, mid2.x, end.x, mid2.y, end.y);


	//float p1 = (mid1.y - start.y) / (mid1.x - start.x);
	//float p2 = (end.y - mid2.y) / (end.x - mid2.x);
	//float a = (p2 - p1) / (mid2.x - mid1.x);
	//float b = p1 - a * mid1.x;

	//float c = mid1.y - a * mid1.x * mid1.x / 2 - b * mid1.x;
	//float diffMid2 = mid2.y - (a * mid2.x * mid2.x / 2 + b * mid2.x + c);
	//
	//float rawResult = a * decibels * decibels / 2 + b * decibels + c;
	//float result = (a * decibels + b) * decibels + 0;//jmap(rawResult, mid1.x, mid2.x, 0.f, 1.f);
	//return result;
}

float DecibelsHelpers::valueToGain(float value)
{
	return Decibels::decibelsToGain(valueToDecibels(value), start.x);
}

float DecibelsHelpers::gainToValue(float gain)
{
	return decibelsToValue(Decibels::gainToDecibels(gain, start.x));
}



VolumeControl::VolumeControl(const String& name, bool hasRMS) :
	ControllableContainer(name),
	prevGain(1),
	rms(nullptr),
	computeRMS(nullptr),
	rmsSampleCount(0),
	rmsMax(0)
{
	editorIsCollapsed = true;

	gain = new DecibelFloatParameter("Gain", "Gain for this");
	addParameter(gain);
	active = addBoolParameter("Active", "Fast way to mute this", true);
	if (hasRMS)
	{
		computeRMS = addBoolParameter("Compute RMS", "Compute RMS for this", true);
		rms = new DecibelFloatParameter("RMS", "RMS for this", 0);
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
	return active->boolValue() ? gain->gain : 0;
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
	if (computeRMS != nullptr && !computeRMS->boolValue()) return;

	if (numSamples == -1) numSamples = buffer.getNumSamples();

	if (rms != nullptr)
	{
		if (channel >= 0) rmsMax = jmax(buffer.getRMSLevel(channel, startSample, numSamples), rmsMax);
		else
		{
			for (int i = 0; i < buffer.getNumChannels(); i++) rmsMax = jmax(buffer.getRMSLevel(i, startSample, numSamples), rmsMax);
		}

		rmsSampleCount += buffer.getNumSamples();
		if (rmsSampleCount > 3000) //~10fps @44100Hz
		{
			//float rmsGainVal = rms->gain + (rmsMax - rms->gain) * (rmsMax > rms->gain ? 1: .4f);
			rms->setGain(rmsMax);

			rmsSampleCount = 0;
			rmsMax = 0;
		}
	}
}