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
	LooperTrack(LooperNode* looper, int index);
	virtual ~LooperTrack();

	LooperNode* looper; //to get the ringbuffer

	static LooperTrack* lastManipulatedTrack;

	enum TrackState { IDLE, WILL_RECORD, RECORDING, FINISH_RECORDING, RETRO_REC, PLAYING, WILL_STOP, STOPPED, WILL_PLAY, STATES_MAX };
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
	IntParameter* numStretchedBeats;


	int index;
	bool firstPlayAfterRecord;
	bool firstPlayAfterStop;

	Transport::Quantization playQuantization;

	int curSample; //for tracking rec and play
	int jumpGhostSample; //when jumping in the sample, keep curSample to fade (force transport playRestart for instance)
	int bufferNumSamples;
	int freeRecStartOffset;
	double timeAtStateChange;
	int retroRecCount;
	bool finishRecordLock;

	float bpmAtRecord;

	int globalBeatAtStart;
	int freePlaySample;
	int numBeats;
	int autoStopRecAfterBeats;

	//stretching
	double stretch;
	int stretchedNumSamples;
	int stretchSample; //for storing one full stretched loop in real time

	virtual void stateChanged();

	virtual void recordOrPlay();
	virtual void startRecording();
	virtual void startRecordingInternal() {}
	virtual void finishRecordingAndPlay();
	virtual void finishRecordingAndPlayInternal() {}
	virtual void retroRecAndPlay();
	virtual void retroRecAndPlayInternal() {}

	virtual void cancelRecording();
	virtual void clearBuffer(bool setIdle = true);
	virtual void startPlaying();
	virtual void stopPlaying();

	virtual void clearTrack();

	virtual void handleWaiting();

	virtual void onContainerTriggerTriggered(Trigger* t) override;
	virtual void onContainerParameterChanged(Parameter* p) override;

	virtual void handleBeatChanged(bool isNewBar, bool isFirstLoop);

	virtual void updateStretch(bool force = false);
	void processTrack(int blockSize, bool forcePlaying = false);


	virtual void loadSampleFile(File f) {}
	virtual void saveSampleFile(File f) {}

	//Helpers
	virtual bool hasContent(bool includeRecordPhase) const;
	bool isRecording(bool includeWillRecord, bool includeRetroRec = false) const;
	bool isPlaying(bool includeWillPlay) const;
	bool isWaiting(bool waitingForRecord = true, bool waitingForFinishRecord = true, bool waitingForPlay = true, bool waitingForStop = true, bool waitingForRetroRec = true) const;
};