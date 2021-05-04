/*
  ==============================================================================

	LooperTrack.h
	Created: 21 Nov 2020 6:33:06pm
	Author:  bkupe

  ==============================================================================
*/

#pragma once


class LooperNode;

class LooperTrack :
	public VolumeControl
{
public:
	LooperTrack(LooperNode * looper, int index);
	virtual ~LooperTrack();

	LooperNode* looper; //to get the ringbuffer

	enum TrackState { IDLE, WILL_RECORD, RECORDING, FINISH_RECORDING, PLAYING, WILL_STOP, STOPPED, WILL_PLAY, STATES_MAX };
	static String trackStateNames[STATES_MAX];

	EnumParameter* trackState;

	Trigger* playRecordTrigger;
	Trigger* playTrigger;
	Trigger* stopTrigger;
	Trigger* clearTrigger;

	BoolParameter* isCurrent;

	IntParameter* loopBeat;
	IntParameter* loopBar;
	FloatParameter* loopProgression;
	IntParameter* section;

	int index;
	bool firstPlayAfterRecord;
	bool firstPlayAfterStop;

	Transport::Quantization playQuantization;

	int curSample; //for tracking rec and play
	int jumpGhostSample; //when jumping in the sample, keep curSample to fade (force transport playRestart for instance)
	int bufferNumSamples;
	int freeRecStartOffset;
	double timeAtStateChange;
	bool finishRecordLock;

	int globalBeatAtStart;
	int freePlaySample;
	int curReadSample;
	int numBeats;

	int autoStopRecAfterBeats;

	virtual void stateChanged();

	virtual void recordOrPlay();
	virtual void startRecording();
	virtual void startRecordingInternal() {}
	virtual void finishRecordingAndPlay();
	virtual void finishRecordingAndPlayInternal() {}
	virtual void cancelRecording();
	virtual void clearBuffer(bool setIdle = true);
	virtual void startPlaying();
	virtual void stopPlaying();

	virtual void clearTrack();

	virtual void handleWaiting();

	virtual void onContainerTriggerTriggered(Trigger* t) override;
	virtual void onContainerParameterChanged(Parameter* p) override;

	virtual void handleBeatChanged(bool isNewBar);

	void processTrack(int blockSize);

	//Helpers
	bool hasContent(bool includeRecordPhase) const;
	bool isRecording(bool includeWillRecord) const;
	bool isPlaying(bool includeWillPlay) const;
	bool isWaiting(bool waitingForRecord = true, bool waitingForFinishRecord = true, bool waitingForPlay = true, bool waitingForStop = true) const;
};