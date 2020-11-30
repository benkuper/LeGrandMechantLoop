/*
  ==============================================================================

	LooperTrack.cpp
	Created: 21 Nov 2020 6:33:06pm
	Author:  bkupe

  ==============================================================================
*/

#include "LooperTrack.h"
#include "LooperNode.h"
#include "Node/NodeManager.h"

String LooperTrack::trackStateNames[LooperTrack::STATES_MAX] = { "Idle", "Will Record", "Recording", "Finish Recording", "Playing", "Will Stop", "Stopped", "Will Play" };

LooperTrack::LooperTrack(LooperProcessor * looper, int index, int numChannels) :
	ControllableContainer("Track " + String(index + 1)),
	looper(looper),
	index(index),
	numChannels(numChannels),
	freeRecStartOffset(0),
	curSample(0),
	globalBeatAtStart(0),
	curPlaySample(0),
	numBeats(0),
	finishRecordLock(false)
{
	editorIsCollapsed = true;
	isSelectable = false; 

	isCurrent = addBoolParameter("Is Current", "Is this track the current one controlled by the looper ?", false);
	isCurrent->hideInEditor = true;

	trackState = addEnumParameter("State", "State of this track, for feedback");
	for (int i = 0; i < STATES_MAX; i++) trackState->addOption(trackStateNames[i], (TrackState)i);
	//trackState->setControllableFeedbackOnly(true);

	playRecordTrigger = addTrigger("Record", "If empty, this will start recording. If recording, this will stop recording and start playing. If already recorded, this will start playing");
	playTrigger = addTrigger("Play", "If empty, this will do nothing. If something is already recording, this will start playing.");
	stopTrigger = addTrigger("Stop", "If recording, this will cancel the recording. If playing, this will stop the track but will keep the recorded content");
	clearTrigger = addTrigger("Clear", "If recording, this will cancel the recording. If playing, this will clear the track of the recorded content");

	active = addBoolParameter("Active", "If this is not checked, this will act as a track mute.", true);
	volume = addFloatParameter("Gain", "The gain for this track", 1, 0, 2);

	loopBeat = addIntParameter("Current Beat", "Current beat of this loop", 0, 0);
	loopBar = addIntParameter("Current Bar", "Current bar of this loop", 0, 0);
	loopProgression = addFloatParameter("Progression", "The progression of this loop", 0, 0, 1);

	rms = addFloatParameter("RMS", "RMS for this track, for feedback", 0, 0, 1);
	rms->setControllableFeedbackOnly(true);

}

LooperTrack::~LooperTrack()
{
}

void LooperTrack::setNumChannels(int num)
{
	if (num == numChannels) return;
	numChannels = num;
	updateBufferSize(buffer.getNumSamples());
	clearBuffer();
}

void LooperTrack::updateBufferSize(int newSize)
{
	buffer.setSize(numChannels, newSize, true, true); 
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
		if (!Transport::getInstance()->isCurrentlyPlaying->boolValue() 
			|| looper->getQuantization() == Transport::FREE)
		{
			startRecording();
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
		curPlaySample = 0; //for freeplaybck
	}
	break;

	case WILL_STOP:
		if (playQuantization == Transport::FREE) stopPlaying();
		break;

	case STOPPED:
	{
		curPlaySample = 0; //for free playback
	}
	break;

	case WILL_PLAY:
		if (!Transport::getInstance()->isCurrentlyPlaying->boolValue()) Transport::getInstance()->play();
		if (playQuantization == Transport::FREE) startPlaying();
		break;
	}
}

void LooperTrack::startRecording()
{
	int recNumSamples = looper->getSampleRate() * 60; // 1 min rec samples
	updateBufferSize(recNumSamples);

	clearBuffer();

	//Store a snapshot of the ring buffer that will be faded at the end of the recorded buffer
	int fadeNumSamples = looper->getFadeNumSamples();
	if (fadeNumSamples > 0)
	{
		preRecBuffer.setSize(buffer.getNumChannels(), fadeNumSamples, false);
		looper->ringBuffer->readSamples(preRecBuffer, fadeNumSamples);
	}

	trackState->setValueWithData(RECORDING);
	if (!Transport::getInstance()->isCurrentlyPlaying->boolValue())
	{
		//If already has content, just play
		bool doSetTempo = !RootNodeManager::getInstance()->hasPlayingNodes();
		Transport::getInstance()->play(doSetTempo);
	}

	Transport::Quantization q = looper->getQuantization();
	if (q == Transport::FREE)
	{
		Transport::Quantization fillMode = looper->getFreeFillMode();
		freeRecStartOffset = 0;
		if (fillMode == Transport::BAR) freeRecStartOffset = Transport::getInstance()->getRelativeBarSamples();
		else if (fillMode == Transport::BEAT) freeRecStartOffset = Transport::getInstance()->getRelativeBeatSamples();
	}
}

void LooperTrack::finishRecordingAndPlay()
{
	Transport::Quantization q = looper->getQuantization();
	Transport::Quantization fillMode = looper->getFreeFillMode();
	
	int beatsPerBar = Transport::getInstance()->beatsPerBar->intValue();
	
	if (Transport::getInstance()->isSettingTempo) Transport::getInstance()->finishSetTempo(true);
	
	
	if (q == Transport::FREE)
	{
		if (fillMode == Transport::BAR) numBeats = jmax(Transport::getInstance()->getBarForSamples(curSample, false), 1) * beatsPerBar;
		else if (fillMode == Transport::BEAT) numBeats = jmax(Transport::getInstance()->getBeatForSamples(curSample, false, false), 1);
		else //fillMode FREE
		{
			loopBeat->setRange(0, numBeats - 1);
			loopBar->setRange(0, 0);
		}
	}
	else
	{
		numBeats = Transport::getInstance()->getBeatForSamples(curSample, false, false);
	}

	if(q != Transport::FREE || (q == Transport::FREE && fillMode != Transport::FREE))
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
	if (isRecording(false)) updateBufferSize(curSample); //update size of buffer, will fill with silence if buffer is larger than what has been recorded

	if (freeRecStartOffset > 0 && q == Transport::FREE && fillMode != Transport::FREE) //This is made to align recorded content to bar/beat by offsetting the data with freeRecStartOffset
	{
		AudioBuffer<float> tmpBuffer(1, buffer.getNumSamples());
		for (int i = 0; i < buffer.getNumChannels(); i++)
		{
			int bufferMinusOffsetSamples = buffer.getNumSamples() - freeRecStartOffset;

			tmpBuffer.copyFrom(0, 0, buffer.getReadPointer(i, bufferMinusOffsetSamples), freeRecStartOffset); //copies end to back
			tmpBuffer.copyFrom(0, freeRecStartOffset, buffer.getReadPointer(i, 0), bufferMinusOffsetSamples);
			buffer.copyFrom(i, 0, tmpBuffer.getReadPointer(0), buffer.getNumSamples());
		}
	}


	if (q != Transport::FREE || (q == Transport::FREE && fillMode == Transport::FREE))
	{
		//fade with ring buffer using looper fadeTimeMS
		int fadeNumSamples = looper->getFadeNumSamples();

		if (fadeNumSamples)
		{
			int bufferStartSample = curSample - 1 - fadeNumSamples;

			buffer.applyGainRamp(bufferStartSample, fadeNumSamples, 1, 0);
			for (int i = 0; i < buffer.getNumChannels(); i++)
			{
				buffer.addFromWithRamp(i, bufferStartSample, preRecBuffer.getReadPointer(i), fadeNumSamples, 0, 1);
			}

			preRecBuffer.clear();
		}
	}

	playQuantization = q != Transport::FREE ? q : fillMode;

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

void LooperTrack::clearBuffer()
{
	buffer.clear();
	curSample = 0;
	trackState->setValueWithData(IDLE);
}

void LooperTrack::startPlaying()
{
	loopBar->setValue(0);
	loopBeat->setValue(0);
	loopProgression->setValue(0);

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

	LOG("[ " << index << " ] Global beat at start " << globalBeatAtStart);
	trackState->setValueWithData(PLAYING);
}

void LooperTrack::stopPlaying()
{
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
	TrackState s = trackState->getValueDataAsEnum<TrackState>();
	if (t == playRecordTrigger)
	{
		recordOrPlay();
	}
	else if (t == playTrigger)
	{
		if (s == STOPPED) trackState->setValue(WILL_PLAY);
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
	TrackState s = trackState->getValueDataAsEnum<TrackState>();
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
		if (!finishRecordLock)
		{
			for (int i = 0; i < numChannels; i++)
			{
				buffer.copyFrom(i, curSample, inputBuffer, i, 0, blockSize);
			}

			curSample += blockSize;
			startReadSample = curSample;

			if (outputIfRecording) outputToMainTrack = true;
		}
	}
	else if(isPlaying(false))
	{
		outputToMainTrack = true;

		if (playQuantization == Transport::FREE)
		{
			if (curPlaySample + blockSize >= buffer.getNumSamples()) curPlaySample = 0;
			startReadSample = curPlaySample;
			curPlaySample += blockSize;
		}
		else //bar, beat
		{
			int curBeat = Transport::getInstance()->getTotalBeatCount() - globalBeatAtStart;
			int trackBeat = curBeat % numBeats;
		
			int relBeatSamples = Transport::getInstance()->getRelativeBeatSamples();
			startReadSample = Transport::getInstance()->getSamplesForBeat(trackBeat, 0, false) + relBeatSamples;

			loopBeat->setValue(trackBeat);
			loopBar->setValue(floor(trackBeat * 1.0f / Transport::getInstance()->beatsPerBar->intValue()));
		}

		loopProgression->setValue(startReadSample * 1.0f / buffer.getNumSamples());
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
