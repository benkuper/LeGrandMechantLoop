/*
  ==============================================================================

	LooperNode.h
	Created: 15 Nov 2020 8:43:03am
	Author:  bkupe

  ==============================================================================
*/

#pragma once

class LooperNode :
	public Node,
	public Transport::TransportListener,
	public EngineListener
{
public:
	enum LooperType { AUDIO, MIDI };

	LooperNode(StringRef name, var params, LooperType looperType);
	virtual ~LooperNode();

	LooperType looperType;

	enum MonitorMode { OFF, ALWAYS, RECORDING_ONLY, ARMED_TRACK };
	EnumParameter* monitorMode;

	LooperTrack* currentTrack;
	ControllableContainer tracksCC;

	ControllableContainer trackParamsCC;
	IntParameter* numTracks;
	IntParameter* currentTrackIndex;
	IntParameter* section;

	ControllableContainer recordCC;
	EnumParameter* recordingState;
	EnumParameter* quantization;
	EnumParameter* freeFillMode;
	IntParameter* fadeTimeMS;
	IntParameter* playStopFadeMS;

	ControllableContainer saveLoadCC;
	enum SaveLoadMode { NONE, LOAD_ONLY, SAVE_ONLY, AUTO_SAVELOAD, AUTO_NO_OVERWRITE };
	EnumParameter* saveLoadMode;
	FileParameter* sampleDirectory;
	Trigger* saveSamplesTrigger;
	Trigger* loadSamplesTrigger;
	Trigger* clearSamplesTrigger;

	enum DoubleRecMode { NOTHING, AUTO_STOP_BAR, AUTO_STOP_BEAT, AUTO_STOP_FIRSTLOOP };
	EnumParameter* doubleRecMode;
	IntParameter* doubleRecVal;
	enum TempMuteMode { NEXT_BAR, NEXT_BEAT, NEXT_FIRSTLOOP };
	EnumParameter* tmpMuteMode;
	FloatParameter* firstRecVolumeThreshold;

	enum RetroRecMode { RETRO_NONE, RETRO_FIRSTLOOP, RETRO_BEAT, RETRO_BAR };
	EnumParameter* retroRecMode;
	IntParameter* retroRecCount;
	IntParameter* retroDoubleRecCount;
	IntParameter* retroTripleRecCount;

	ControllableContainer controlsCC;
	Trigger* recTrigger;
	Trigger* retroRecTrigger;
	Trigger* clearCurrentTrigger;
	Trigger* playAllTrigger;
	Trigger* playCurrentSectionTrigger;

	Trigger* stopAllTrigger;

	Trigger* clearSectionTrigger;
	Trigger* clearOtherSectionsTrigger;
	Trigger* clearAllTrigger;
	Trigger* tmpMuteAllTrigger;

	//View
	BoolParameter* showGlobalControl;
	BoolParameter* showTracks;
	BoolParameter* showTrackActives;
	BoolParameter* showTrackGains;
	BoolParameter* showTrackRMS;
	BoolParameter* showTrackRec;
	BoolParameter* showTrackStopClear;

	//tmp mute
	Array<LooperTrack*> tmpMuteTracks;

	virtual void initInternal() override;

	virtual void updateLooperTracks();
	virtual LooperTrack* createLooperTrack(int index) { return nullptr; }

	virtual void setCurrentTrack(LooperTrack* t);

	virtual void onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c) override;

	virtual void bpmChanged() override;
	virtual void beatChanged(bool isNewBar, bool isFirstLoopBeat) override;
	virtual void playStateChanged(bool isPlaying, bool forceRestart) override;

	virtual void loadSamples();
	virtual void saveSamples();
	virtual void clearSamples();

	//helpers
	virtual bool hasContent(bool includeFreeTracks = true);
	virtual bool areAllTrackedStopped(bool includeFreeTracks = true);
	virtual bool isOneTrackRecording(bool includeWillRecord = false);
	LooperTrack* getTrackForIndex(int index);
	LooperTrack* getFirstEmptyTrack();
	void setCurrentTrackToFirstEmpty();

	Array<LooperTrack*> getTracksForSection(int section);
	Array<LooperTrack*> getTracksExceptSection(int section);

	Transport::Quantization getQuantization();
	Transport::Quantization getFreeFillMode();

	int getRetroBeatMultiplier() const;
	int getRetroNumBeats(int retroCount);
	int getRetroNumSamples(int retroCount);


	var getJSONData() override;
	void loadJSONDataItemInternal(var data) override;

	void fileSaved(bool) override;
	void endLoadFile() override;

	BaseNodeViewUI* createViewUI() override;
};
