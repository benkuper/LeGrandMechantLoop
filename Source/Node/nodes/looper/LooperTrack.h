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
	Trigger* playTrigger;
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

	int globalBeatAtStart;
	int numBeats;

	void setNumChannels(int num);
	void updateAudioBuffer();
	
	void stateChanged();

	void recordOrPlay();
	void startRecording();
	void finishRecordingAndPlay();
	void cancelRecording();
	void clearBuffer();
	void startPlaying();
	void stopPlaying();

	void handleWaiting();

	void onContainerTriggerTriggered(Trigger* t) override;
	void onContainerParameterChanged(Parameter* p) override;

	void handleBeatChanged(bool isNewBar);
	void processBlock(AudioBuffer<float>& inputBuffer, AudioBuffer<float>&outputBuffer, int numMainChannels, bool outputIfRecording);

	//Helpers
	bool isRecording(bool includeWillRecord) const;
	bool isPlaying(bool includeWillPlay) const;
	bool isWaiting(bool waitingForRecord = true, bool waitingForFinishRecord = true, bool waitingForPlay = true, bool waitingForStop = true) const;
};