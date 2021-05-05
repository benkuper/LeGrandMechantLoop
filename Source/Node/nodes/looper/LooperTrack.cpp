/*
  ==============================================================================

	LooperTrack.cpp
	Created: 21 Nov 2020 6:33:06pm
	Author:  bkupe

  ==============================================================================
*/

String LooperTrack::trackStateNames[LooperTrack::STATES_MAX] = { "Idle", "Will Record", "Recording", "Finish Recording", "Playing", "Will Stop", "Stopped", "Will Play" };

LooperTrack::LooperTrack(LooperNode * looper, int index) :
	VolumeControl(String(index + 1), false),
	looper(looper),
	index(index),
	firstPlayAfterRecord(false),
	firstPlayAfterStop(false),
    curSample(0),
	jumpGhostSample(-1),
    bufferNumSamples(0),
    freeRecStartOffset(0),
    timeAtStateChange(0),
    finishRecordLock(false),
	globalBeatAtStart(0),
	freePlaySample(0),
	numBeats(0),
	autoStopRecAfterBeats(-1)
{
	saveAndLoadRecursiveData = true;
	editorIsCollapsed = true;

	isCurrent = addBoolParameter("Is Current", "Is this track the current one controlled by the looper ?", false);
	isCurrent->hideInEditor = true;
	isCurrent->setControllableFeedbackOnly(true);

	trackState = addEnumParameter("State", "State of this track, for feedback");
	for (int i = 0; i < STATES_MAX; i++) trackState->addOption(trackStateNames[i], (TrackState)i);
	trackState->setControllableFeedbackOnly(true);

	playRecordTrigger = addTrigger("Record", "If empty, this will start recording. If recording, this will stop recording and start playing. If already recorded, this will start playing");
	playTrigger = addTrigger("Play", "If empty, this will do nothing. If something is already recording, this will start playing.");
	stopTrigger = addTrigger("Stop", "If recording, this will cancel the recording. If playing, this will stop the track but will keep the recorded content");
	clearTrigger = addTrigger("Clear", "If recording, this will cancel the recording. If playing, this will clear the track of the recorded content");

	loopBeat = addIntParameter("Current Beat", "Current beat of this loop", 0, 0);
	loopBeat->setControllableFeedbackOnly(true);
	loopBeat->defaultHideInRemoteControl = true;
	loopBeat->hideInRemoteControl = true;

	loopBar = addIntParameter("Current Bar", "Current bar of this loop", 0, 0);
	loopBar->setControllableFeedbackOnly(true);
	loopBar->defaultHideInRemoteControl = true;
	loopBar->hideInRemoteControl = true;

	loopProgression = addFloatParameter("Progression", "The progression of this loop", 0, 0, 1);
	loopProgression->setControllableFeedbackOnly(true);
	loopProgression->defaultHideInRemoteControl = true;
	loopProgression->hideInRemoteControl = true;

	section = addIntParameter("Section", "The section this track was recorded for. This is for musical structure purpose", 1, 1);
}

LooperTrack::~LooperTrack()
{
}


void LooperTrack::recordOrPlay()
{
	TrackState s = trackState->getValueDataAsEnum<TrackState>();

	switch (s)
	{
	case IDLE:
	{
		trackState->setValueWithData(WILL_RECORD);
	}
	break;

	case RECORDING:
	{
		trackState->setValueWithData(FINISH_RECORDING);
	}
	break;

	case STOPPED:
	{
		trackState->setValueWithData(WILL_PLAY);
	}
	break;

	case WILL_STOP:
	{
		trackState->setValueWithData(PLAYING); //was alreayd playing, cancel the will stop and keep playing
	}
	break;

	default:
		break;
	}
}

void LooperTrack::stateChanged()
{
	TrackState s = trackState->getValueDataAsEnum<TrackState>();

	switch (s)
	{
	case IDLE:

		break;

	case WILL_RECORD:
	{

		if (!Transport::getInstance()->isCurrentlyPlaying->boolValue()
			|| looper->getQuantization() == Transport::FREE)
		{
			if (!looper->firstRecVolumeThreshold->enabled) startRecording();
		}
	}
	break;

	case RECORDING:
	{
	}
	break;

	case FINISH_RECORDING:
	{
		if (!Transport::getInstance()->isCurrentlyPlaying->boolValue()
			|| looper->getQuantization() == Transport::FREE)
		{
			finishRecordingAndPlay();
		}
	}

	case PLAYING:
	{
		freePlaySample = 0; //for freeplaybck
	}
	break;

	case WILL_STOP:
		if (playQuantization == Transport::FREE) stopPlaying();
		break;

	case STOPPED:
	{
		freePlaySample = 0;
	}
	break;

	case WILL_PLAY:
		firstPlayAfterStop = true;

		if (!Transport::getInstance()->isCurrentlyPlaying->boolValue())
		{
			Transport::getInstance()->play();
			startPlaying();
		}

		if (playQuantization == Transport::FREE) startPlaying();
		break;
            
        default:
            break;
	}
}

void LooperTrack::startRecording()
{
	startRecordingInternal();
	
	trackState->setValueWithData(RECORDING);

	Transport::Quantization q = looper->getQuantization();

	if (q == Transport::FREE)
	{
		Transport::Quantization fillMode = looper->getFreeFillMode();
		freeRecStartOffset = 0;
		if (fillMode == Transport::BAR) freeRecStartOffset = Transport::getInstance()->getRelativeBarSamples();
		else if (fillMode == Transport::BEAT) freeRecStartOffset = Transport::getInstance()->getRelativeBeatSamples();
	}
	else
	{
		if (!Transport::getInstance()->isCurrentlyPlaying->boolValue())
		{
			//If already has content, just play
			bool doSetTempo = !RootNodeManager::getInstance()->hasPlayingNodes();
			Transport::getInstance()->play(doSetTempo);
		}
	}
}

void LooperTrack::finishRecordingAndPlay()
{
	autoStopRecAfterBeats = -1;
	
	Transport::Quantization q = looper->getQuantization();
	Transport::Quantization fillMode = looper->getFreeFillMode();
	
	int beatsPerBar = Transport::getInstance()->beatsPerBar->intValue();
	
	bool isFullFree = q == Transport::FREE && fillMode == Transport::FREE;

	if (Transport::getInstance()->isSettingTempo)
	{
		Transport::getInstance()->finishSetTempo(true);
	}

	if (q == Transport::FREE)
	{
		if (fillMode == Transport::BAR) numBeats = jmax(Transport::getInstance()->getBarForSamples(curSample, false), 1) * beatsPerBar;
		else if (fillMode == Transport::BEAT) numBeats = jmax(Transport::getInstance()->getBeatForSamples(curSample, false, false), 1);
		else //fillMode FREE
		{
			numBeats = -1;
		}
	}
	else
	{
		numBeats = Transport::getInstance()->getBeatForSamples(curSample, false, false);
	}

	if (numBeats == 0)
	{
		NLOGWARNING(looper->niceName, "Error recording loop");
		clearBuffer();
		return;
	}

	if(!isFullFree)
	{
		loopBeat->setRange(0, numBeats - 1);
		loopBar->setRange(0, jmax<int>(floor(numBeats * 1.0f / beatsPerBar) - 1, 0));

		int curSamplePerfect = numBeats * Transport::getInstance()->getBeatNumSamples();
		curSample = curSamplePerfect;
	}
	else
	{
		curSample = Transport::getInstance()->getBlockPerfectNumSamples(curSample, false); //make it blockPerfect
	}

	// if curSample > perfect, use end of curSample to fade with start ?
	// if curSample < perfect, use phantom buffer to fill in blank ?


	finishRecordLock = true;
	bufferNumSamples = curSample;

	finishRecordingAndPlayInternal();

	playQuantization = q != Transport::FREE ? q : fillMode;

	firstPlayAfterRecord = true;
	curSample = 0; //force here to avoid jumpGhost on rec
	startPlaying();

	finishRecordLock = false;
}

void LooperTrack::cancelRecording()
{
	if (Transport::getInstance()->isSettingTempo)
	{
		Transport::getInstance()->stop();
	}

	clearBuffer();
}

void LooperTrack::clearBuffer(bool setIdle)
{
	curSample = 0;
	bufferNumSamples = 0;
	jumpGhostSample = -1;
	if(setIdle) trackState->setValueWithData(IDLE);
}

void LooperTrack::startPlaying()
{
	loopBar->setValue(0);
	loopBeat->setValue(0);
	loopProgression->setValue(0);
	if (curSample > 0) jumpGhostSample = curSample;
	else jumpGhostSample = -1;
	curSample = 0;

	Transport::Quantization q = looper->getQuantization();
	Transport::Quantization fillMode = looper->getFreeFillMode();

	if (q == Transport::BAR || (q == Transport::FREE && fillMode == Transport::BAR))
	{
		globalBeatAtStart = Transport::getInstance()->curBar->intValue() * Transport::getInstance()->beatsPerBar->intValue(); //snap to bar
	}
	else
	{
		globalBeatAtStart = Transport::getInstance()->getTotalBeatCount();
	}

	//LOG("[ " << index << " ] Global beat at start " << globalBeatAtStart);
	trackState->setValueWithData(PLAYING);
}

void LooperTrack::stopPlaying()
{
	if (isRecording(true)) cancelRecording();
	else if (isPlaying(true)) trackState->setValueWithData(STOPPED);
	curSample = 0;
	jumpGhostSample = -1;
}

void LooperTrack::clearTrack()
{
	if (isRecording(true)) cancelRecording();
	clearBuffer();
	resetGainAndActive();
	section->setValue(1);
}

void LooperTrack::handleWaiting()
{
	TrackState s = trackState->getValueDataAsEnum<TrackState>();

	switch (s)
	{
	case WILL_RECORD: startRecording(); break;
	case FINISH_RECORDING: finishRecordingAndPlay(); break;
	case WILL_PLAY: startPlaying(); break;
	case WILL_STOP: stopPlaying(); break;
	default: break;
	}
}

void LooperTrack::onContainerTriggerTriggered(Trigger* t)
{
	TrackState s = trackState->getValueDataAsEnum<TrackState>();
	if (t == playRecordTrigger)
	{
		recordOrPlay();
	}
	else if (t == playTrigger)
	{
		if (s == STOPPED) trackState->setValueWithData(WILL_PLAY);
		else if (s == WILL_STOP) trackState->setValue(WILL_STOP); //meaning it was already playing, cancel the will stop and keep playing
	}
	else if (t == stopTrigger)
	{
		if (isRecording(true))
		{
			cancelRecording();
			trackState->setValueWithData(IDLE);
		}
		else if(s == PLAYING) trackState->setValueWithData(WILL_STOP);
		else if (s == WILL_PLAY) trackState->setValueWithData(STOPPED);
	}
	else if (t == clearTrigger)
	{
		clearTrack();
	}
}

void LooperTrack::onContainerParameterChanged(Parameter* p)
{
	if (p == trackState)
	{
		stateChanged();
	}
}
void LooperTrack::handleBeatChanged(bool isNewBar)
{
	TrackState s = trackState->getValueDataAsEnum<TrackState>();
	if (s == RECORDING && autoStopRecAfterBeats > 0)
	{
		autoStopRecAfterBeats--;
		if (autoStopRecAfterBeats == 0) finishRecordingAndPlay();
		return;
	}


	if (!isWaiting()) return;
	
	if (s == IDLE) return;
	
	if (isRecording(true))
	{
		Transport::Quantization q = looper->getQuantization();
		if ((q == Transport::BAR && isNewBar) || (q == Transport::BEAT)) handleWaiting();
	}
	else if (isPlaying(true))
	{
		if ((playQuantization == Transport::BAR && isNewBar) || (playQuantization == Transport::BEAT)) handleWaiting();
	}
	
}

void LooperTrack::processTrack(int blockSize, bool forcePlaying)
{
	//int startReadSample = 0;

	if (isRecording(false))
	{
		if (!finishRecordLock)
		{
			curSample += blockSize;
			//startReadSample = curSample;
		}
	}
	else if (isPlaying(false))
	{
		if (playQuantization == Transport::FREE)
		{
			if (freePlaySample >= bufferNumSamples) freePlaySample = 0;
			curSample = freePlaySample;
			freePlaySample += blockSize;

		}
		else //bar, beat
		{
			if (!forcePlaying && !Transport::getInstance()->isCurrentlyPlaying->boolValue()) return;
			
			int curBeat = Transport::getInstance()->getTotalBeatCount() - globalBeatAtStart;
			int trackBeat = curBeat % numBeats;
			/**
			int relBeatSamples = Transport::getInstance()->getRelativeBeatSamples();
			startReadSample = Transport::getInstance()->getSamplesForBeat(trackBeat, 0, false) + relBeatSamples;
			*/

			loopBeat->setValue(trackBeat);
			loopBar->setValue(floor(trackBeat * 1.0f / Transport::getInstance()->beatsPerBar->intValue()));

			if (curSample >= bufferNumSamples) curSample = 0; 
			else curSample += blockSize;
			jassert(curSample <= bufferNumSamples);

			if (curSample >= bufferNumSamples)
			{
				curSample = 0;
				firstPlayAfterStop = false;
			}

			//startReadSample = curSample;
		}

		loopProgression->setValue(curSample * 1.0f / bufferNumSamples);
		//curSample = startReadSample;
	}
}

bool LooperTrack::hasContent(bool includeRecordPhase) const
{
	TrackState s = trackState->getValueDataAsEnum<TrackState>();
	if (s == IDLE) return false;
	if (isRecording(true)) return includeRecordPhase;
	return true;
}

bool LooperTrack::isRecording(bool includeWillRecord) const
{
	TrackState s = trackState->getValueDataAsEnum<TrackState>();
	return s == RECORDING || s == FINISH_RECORDING || (includeWillRecord && s == WILL_RECORD);
}

bool LooperTrack::isPlaying(bool includeWillPlay) const
{
	TrackState s = trackState->getValueDataAsEnum<TrackState>();
	return s == PLAYING || s == WILL_STOP || (includeWillPlay && s == WILL_PLAY);
}

bool LooperTrack::isWaiting(bool waitingForRecord, bool waitingForFinishRecord, bool waitingForPlay, bool waitingForStop) const
{
	TrackState s = trackState->getValueDataAsEnum<TrackState>();
	return (waitingForRecord && s == WILL_RECORD) || (waitingForFinishRecord && s == FINISH_RECORDING) || (waitingForPlay && s == WILL_PLAY) || (waitingForStop && s == WILL_STOP);
}