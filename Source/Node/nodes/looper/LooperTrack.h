/*
  ==============================================================================

	LooperTrack.h
	Created: 21 Nov 2020 6:33:06pm
	Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"
#include "Common/RingBuffer.h"
#include "Transport/Transport.h"

class LooperProcessor;

class LooperTrack :
	public ControllableContainer
{
public:
	LooperTrack(LooperProcessor * looper, int index);
	virtual ~LooperTrack();

	LooperProcessor* looper; //to get the ringbuffer

	enum TrackState { IDLE, WILL_RECORD, RECORDING, FINISH_RECORDING, PLAYING, WILL_STOP, STOPPED, WILL_PLAY, STATES_MAX };
	static String trackStateNames[STATES_MAX];

	EnumParameter* trackState;

	Trigger* playRecordTrigger;
	Trigger* playTrigger;
	Trigger* stopTrigger;
	Trigger* clearTrigger;

	BoolParameter* isCurrent;
	BoolParameter* active;

	IntParameter* loopBeat;
	IntParameter* loopBar;
	FloatParameter* loopProgression;

	int index;

	Transport::Quantization playQuantization;

	int curSample;
	int freeRecStartOffset;
	double timeAtStateChange;
	bool finishRecordLock;

	int globalBeatAtStart;
	int curPlaySample;
	int numBeats;

	
	virtual void stateChanged();

	virtual void recordOrPlay();
	virtual void startRecording();
	virtual void startRecordingInternal() {}
	virtual void finishRecordingAndPlay();
	virtual void finishRecordingAndPlayInternal() {}
	virtual void cancelRecording();
	virtual void clearBuffer();
	virtual void startPlaying();
	virtual void stopPlaying();

	virtual void handleWaiting();

	virtual void onContainerTriggerTriggered(Trigger* t) override;
	virtual void onContainerParameterChanged(Parameter* p) override;

	virtual void handleBeatChanged(bool isNewBar);

	//Helpers
	bool hasContent(bool includeRecordPhase) const;
	bool isRecording(bool includeWillRecord) const;
	bool isPlaying(bool includeWillPlay) const;
	bool isWaiting(bool waitingForRecord = true, bool waitingForFinishRecord = true, bool waitingForPlay = true, bool waitingForStop = true) const;
};