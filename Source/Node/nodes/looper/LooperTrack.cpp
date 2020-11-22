/*
  ==============================================================================

	LooperTrack.cpp
	Created: 21 Nov 2020 6:33:06pm
	Author:  bkupe

  ==============================================================================
*/

#include "LooperTrack.h"
#include "Transport/Transport.h"

String LooperTrack::trackStateNames[LooperTrack::STATES_MAX] = { "Idle", "Will Record", "Recording", "Finish Recording", "Playing", "Will Stop", "Stopped", "Will Play" };

LooperTrack::LooperTrack(int index, int numChannels) :
	ControllableContainer("Track " + String(index + 1)),
	index(index),
	numChannels(numChannels),
	curSample(0),
	globalBeatAtStart(0),
	numBeats(0)
{
	editorIsCollapsed = true;
	isSelectable = false;

	trackState = addEnumParameter("State", "State of this track, for feedback");
	for (int i = 0; i < STATES_MAX; i++) trackState->addOption(trackStateNames[i], (TrackState)i);
	//trackState->setControllableFeedbackOnly(true);

	playRecordTrigger = addTrigger("Record", "If empty, this will start recording. If recording, this will stop recording and start playing. If already recorded, this will start playing");
	playTrigger = addTrigger("Play", "If empty, this will do nothing. If something is already recording, this will start playing.");
	stopTrigger = addTrigger("Stop", "If recording, this will cancel the recording. If playing, this will stop the track but will keep the recorded content");
	clearTrigger = addTrigger("Clear", "If recording, this will cancel the recording. If playing, this will clear the track of the recorded content");

	active = addBoolParameter("Active", "If this is not checked, this will act as a track mute.", true);
	volume = addFloatParameter("Gain", "The gain for this track", 1, 0, 2);
	rms = addFloatParameter("RMS", "RMS for this track, for feedback", 0, 0, 1);
	rms->setControllableFeedbackOnly(true);

	updateAudioBuffer();
}

LooperTrack::~LooperTrack()
{
}

void LooperTrack::setNumChannels(int num)
{
	if (num == numChannels) return;
	numChannels = num;
	updateAudioBuffer();
	clearBuffer();
}

void LooperTrack::updateAudioBuffer()
{
	int numSamples = curSample;
	if(trackState->getValueDataAsEnum<TrackState>() == RECORDING) numSamples = Transport::getInstance()->getBarNumSamples() * 10; //10 bar by default when recording
	buffer.setSize(numChannels, numSamples, true, true); 
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
		if (!Transport::getInstance()->isCurrentlyPlaying->boolValue()) startRecording();
	}
	break;

	case RECORDING:
	{
		updateAudioBuffer();
	}
	break;

	case FINISH_RECORDING:
	{
		if (!Transport::getInstance()->isCurrentlyPlaying->boolValue()) finishRecordingAndPlay();
	}

	case PLAYING:
	{
	}
	break;

	case WILL_STOP:
		break;

	case STOPPED:
		break;

	case WILL_PLAY:
		if (!Transport::getInstance()->isCurrentlyPlaying->boolValue()) Transport::getInstance()->play();
		break;
	}
}

void LooperTrack::startRecording()
{
	clearBuffer();
	trackState->setValueWithData(RECORDING);
	if (!Transport::getInstance()->isCurrentlyPlaying->boolValue())
	{
		Transport::getInstance()->play(true);
	}
}

void LooperTrack::finishRecordingAndPlay()
{
	if (!Transport::getInstance()->isCurrentlyPlaying->boolValue()) Transport::getInstance()->finishSetTempo(true);

	numBeats = Transport::getInstance()->getBeatForSamples(curSample, false, false);
	int curSamplePerfect = numBeats * Transport::getInstance()->getBeatNumSamples();

	LOG("Finished recording, Num beats : " << numBeats << ", curSample : " << curSample << ", beatSamples : " << Transport::getInstance()->getBeatNumSamples());

	curSample = curSamplePerfect;

	if (isRecording(false)) updateAudioBuffer();
	startPlaying();
}

void LooperTrack::cancelRecording()
{
	if (Transport::getInstance()->isSettingTempo)
	{
		Transport::getInstance()->stop();
	}
	clearBuffer();
}

void LooperTrack::clearBuffer()
{
	buffer.clear();
	curSample = 0;
	trackState->setValueWithData(IDLE);
}

void LooperTrack::startPlaying()
{
	curSample = 0;
	globalBeatAtStart = Transport::getInstance()->getTotalBeatCount();
	LOG("[ " << index << " ] Global beat at start " << globalBeatAtStart);
	trackState->setValueWithData(PLAYING);
}

void LooperTrack::stopPlaying()
{
	curSample = 0;
	if (isRecording(true)) cancelRecording();
	else if (isPlaying(true)) trackState->setValueWithData(STOPPED);
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
	if (t == playRecordTrigger)
	{
		recordOrPlay();
	}
	else if (t == playTrigger)
	{
		if (trackState->getValueDataAsEnum<TrackState>() == STOPPED) trackState->setValue(WILL_PLAY);
	}
	else if (t == stopTrigger)
	{
		if (isRecording(true))
		{
			cancelRecording();
			trackState->setValueWithData(IDLE);
		}
		else
		{
			trackState->setValueWithData(WILL_STOP);
		}
	}
	else if (t == clearTrigger)
	{
		if (isRecording(true)) cancelRecording(); 
		clearBuffer();
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
	if (!isWaiting()) return;
	Transport::Quantization q = Transport::getInstance()->quantization->getValueDataAsEnum<Transport::Quantization>();
	TrackState s = trackState->getValueDataAsEnum<TrackState>();
	if (s == IDLE) return;
	if((q == Transport::BAR && isNewBar) || (q == Transport::BEAT)) handleWaiting();
}

void LooperTrack::processBlock(AudioBuffer<float>& inputBuffer, AudioBuffer<float>& outputBuffer, int numMainChannels, bool outputIfRecording)
{
	float rmsVal = 0;

	bool outputToMainTrack = false;
	int trackChannel = numMainChannels + index;
	bool outputToSeparateTrack = outputBuffer.getNumChannels() > trackChannel;

	int startReadSample = 0;
	int blockSize = inputBuffer.getNumSamples();

	if (isRecording(false))
	{
		for (int i = 0; i < numChannels; i++)
		{
			buffer.copyFrom(i, curSample, inputBuffer, i, 0, blockSize);
		}

		curSample += blockSize;
		startReadSample = curSample;

		if (outputIfRecording) outputToMainTrack = true;
	}
	else if(isPlaying(false))
	{
		outputToMainTrack = true;

		int curBeat = Transport::getInstance()->getTotalBeatCount() - globalBeatAtStart;
		int trackBeat = (curBeat % numBeats);
		int relBeatSamples = Transport::getInstance()->getRelativeBeatSamples();
		startReadSample = Transport::getInstance()->getSamplesForBeat(trackBeat, 0, false) + relBeatSamples;
		DBG("[ " << index << " ] " << " cur beat : " << curBeat << " ( " << globalBeatAtStart << " ) "  << Transport::getInstance()->getTotalBeatCount());
	}


	if (outputToSeparateTrack)
	{
		outputBuffer.clear(trackChannel, 0, blockSize);
	}

	if ((outputToMainTrack || outputToSeparateTrack) && (startReadSample <= buffer.getNumSamples()))
	{
		for (int i = 0; i < numChannels; i++)
		{
			if (outputToMainTrack)
			{
				outputBuffer.addFrom(i, 0, buffer, i, startReadSample, blockSize, volume->floatValue());
			}

			if (outputToSeparateTrack)
			{
				outputBuffer.addFrom(trackChannel, 0, buffer, i, startReadSample, blockSize, volume->floatValue());
			}

			rmsVal = jmax(rmsVal, buffer.getRMSLevel(i, startReadSample, blockSize));
		}

		float curVal = rms->floatValue();
		float targetVal = rms->getLerpValueTo(rmsVal, rmsVal > curVal ? .8f : .2f);
		rms->setValue(targetVal);
	}
	else
	{
		rms->setValue(0);
	}

	


	/*
	//tmp
	int relBarSamples = Transport::getInstance()->timeInSamples % Transport::getInstance()->getBarNumSamples();

	//tracks
	for (int i = 0; i < numTracks->intValue(); i++)
	{
		LooperTrack* t = (LooperTrack*)tracksCC.controllableContainers[i].get();
		bool includeSource = (mm == RECORDING_ONLY && t->isRecording(false)) || t->isPlaying(false);
		if (includeSource)
		{
			for (int i = 0; i < trackStartChannel; i++)
			{
				buffer.addFrom(i, 0, t->buffer, i, relBarSamples, buffer.getNumSamples());
			}

			if (tom == SEPARATE_ONLY || tom == ALL)
			{
				buffer.copyFrom(trackStartChannel + i, 0, t->buffer, i, relBarSamples, buffer.getNumSamples());
			}
		}
	}
	*/
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
