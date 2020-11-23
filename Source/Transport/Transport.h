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

	FloatParameter* bpm;
	IntParameter* beatsPerBar; //first part of time signature
	IntParameter* beatUnit; //2nd part of time signature

	BoolParameter* isCurrentlyPlaying;
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
	int blockSize;

	int timeInSamples;
	int numSamplesPerBeat;

	bool isSettingTempo;
	int setTempoSampleCount;

	double timeAtStart;

	void clear(); // override here to avoid deleting parameters

	//Controls
	void play(bool startTempoSet = false);
	void pause();
	void stop();

	void finishSetTempo(bool startPlaying = true);

	void setCurrentTime(int samples);
	void gotoBar(int bar);
	void gotoBeat(int beat, int bar = -1);

	//Events
	void onContainerTriggerTriggered(Trigger* t) override;
	void onContainerParameterChanged(Parameter* p) override;

	//Helpers
	int getBarNumSamples() const;
	int getBeatNumSamples() const;
	int getSamplesToNextBar() const;
	int getSamplesToNextBeat() const;
	int getRelativeBarSamples() const;
	int getRelativeBeatSamples() const;

	int getBarForSamples(int samples, bool floorResult = true) const;
	int getBeatForSamples(int samples, bool relative = true, bool floorResult = true) const;
	int getSamplesForBar(int bar = -1) const;
	int getSamplesForBeat(int beat = -1, int bar = -1, bool relative = true) const;

	double getBarLength() const;
	double getBeatLength() const;
	double getTimeToNextBar() const;
	double getTimeToNextBeat() const;
	double getTimeForSamples(int samples) const;

	double getCurrentTime() const;
	double getTimeForBar(int bar = -1) const;
	double getTimeForBeat(int beat = -1, int bar = -1, bool relative = true) const;
	int getBarForTime(double time, bool floorResult = true) const;
	int getBeatForTime(double time, bool relative = true, bool floorResult = true) const;

	int getSamplesForTime(double time, bool blockPerfect = true) const;
	int getTotalBeatCount() const;

	// Inherited via AudioIODeviceCallback
	virtual void audioDeviceIOCallback(const float** inputChannelData, int numInputChannels, float** outputChannelData, int numOutputChannels, int numSamples) override;
	virtual void audioDeviceAboutToStart(AudioIODevice* device) override;
	virtual void audioDeviceStopped() override;

	// Inherited via AudioPlayHead
	//virtual bool getCurrentPosition(CurrentPositionInfo& result) override;

	class TransportListener
	{
	public:
		virtual ~TransportListener() {}
		virtual void beatNumSamplesChanged() {}
		virtual void beatChanged(bool isNewBar) {} //this allow for event after both bar and beat have been updated
		virtual void playStateChanged(bool isPlaying) {}
	};

	ListenerList<TransportListener> transportListeners;
	void addTransportListener(TransportListener* newListener) { transportListeners.add(newListener); }
	void removeTransportListener(TransportListener* listener) { transportListeners.remove(listener); }


	//DECLARE_ASYNC_EVENT(Transport, Transport, transport, ENUM_LIST(TIME_SIGNATURE_CHANGED, BPM_CHANGED))
};