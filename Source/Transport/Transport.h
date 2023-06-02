/*
  ==============================================================================

	Transport.h
	Created: 15 Nov 2020 8:53:43am
	Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"

#if USE_ABLETONLINK
#pragma warning(push)
#pragma warning(disable:4996)
#include <ableton/Link.hpp>
#include <ableton/link/HostTimeFilter.hpp>
#pragma warning(pop)
#endif

class Transport :
	public ControllableContainer,
	public AudioIODeviceCallback,
	public AudioPlayHead
{
public:
	juce_DeclareSingleton(Transport, true);

	Transport();
	~Transport();

	enum Quantization { BAR, BEAT, FREE, DEFAULT, FIRSTLOOP };
	EnumParameter* quantization;

	enum RecQuantization { REC_AUTO, REC_BAR, REC_BEAT, REC_FIRSTLOOP };
	EnumParameter* recQuantization;
	IntParameter* recQuantizCount;
	Point2DParameter* recQuantizBPMRange;

	bool settingBPMFromTransport;
	FloatParameter* bpm;
	IntParameter* beatsPerBar; //first part of time signature
	IntParameter* beatUnit; //2nd part of time signature

	BoolParameter* isCurrentlyPlaying;
	FloatParameter* currentTime;
	IntParameter* curBar;
	IntParameter* curBeat;
	FloatParameter* barProgression;
	FloatParameter* beatProgression;
	IntParameter* firstLoopBeats;
	FloatParameter* firstLoopProgression;

	Trigger* playTrigger;
	Trigger* togglePlayTrigger;
	Trigger* pauseTrigger;
	Trigger* stopTrigger;

	BoolParameter* useAbletonLink;
	IntParameter* numLinkClients;

	int sampleRate;
	int blockSize;

	int64 timeInSamples;
	int numSamplesPerBeat;

	bool isSettingTempo;
	int setTempoSampleCount;

	double timeAtStart;

#if USE_ABLETONLINK
	std::unique_ptr<ableton::Link> link;
#endif

	void clear() override; // override here to avoid deleting parameters

	//Controls
	void play(bool startTempoSet = false, bool playFromStart = false);
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
	int64 getRelativeBarSamples() const;
	int64 getRelativeBeatSamples() const;
	int64 getBlockPerfectNumSamples(int64 samples, bool floorResult = true) const;

	int getBarForSamples(int64 samples, bool floorResult = true) const;
	int getBeatForSamples(int64 samples, bool relative = true, bool floorResult = true) const;
	int getSamplesForBar(int bar = -1) const;
	int getSamplesForBeat(int beat = -1, int bar = -1, bool relative = true) const;

	double getBarLength() const;
	double getBeatLength() const;
	double getTimeToNextBar() const;
	double getTimeToNextBeat() const;
	double getRelativeFirstLoopTime() const;
	double getTimeToNextFirstLoop() const;
	double getTimeForSamples(int samples) const;

	double getCurrentTime() const;
	double getCurrentTimeInBeats() const;
	double getCurrentTimeInBars() const;
	double getTimeForBar(int bar = -1) const;
	double getTimeForBeat(int beat = -1, int bar = -1, bool relative = true) const;

	int getBarForTime(double time, bool floorResult = true) const;
	int getBeatForTime(double time, bool relative = true, bool floorResult = true) const;

	int getSamplesForTime(double time, bool blockPerfect = true) const;
	int getTotalBeatCount() const;

	void setupAbletonLink();

	// Inherited via AudioIODeviceCallback

	virtual void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
			int numInputChannels,
			float* const* outputChannelData,
			int numOutputChannels,
			int numSamples,
			const AudioIODeviceCallbackContext& context) override;

	virtual void audioDeviceAboutToStart(AudioIODevice* device) override;
	virtual void audioDeviceStopped() override;

	// Inherited via AudioPlayHead
	Optional<PositionInfo> getPosition() const override;

	class TransportListener
	{
	public:
		virtual ~TransportListener() {}
		virtual void beatNumSamplesChanged() {}
		virtual void bpmChanged() {}
		virtual void beatChanged(bool isNewBar, bool isFirstLoopBeat) {} //this allow for event after both bar and beat have been updated. 
		//isFirstLoopBeat will be true if the current beat is the start of the first loop that set the tempo
		virtual void playStateChanged(bool isPlaying, bool forceRestart) {}
	};

	ListenerList<TransportListener> transportListeners;
	void addTransportListener(TransportListener* newListener) { transportListeners.add(newListener); }
	void removeTransportListener(TransportListener* listener) { transportListeners.remove(listener); }

	//DECLARE_ASYNC_EVENT(Transport, Transport, transport, ENUM_LIST(TIME_SIGNATURE_CHANGED, BPM_CHANGED))
};
