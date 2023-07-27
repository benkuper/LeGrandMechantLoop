/*
  ==============================================================================

	AudioLooperNode.cpp
	Created: 1 Dec 2020 11:16:14pm
	Author:  bkupe

  ==============================================================================
*/

#include "Node/NodeIncludes.h"

AudioLooperNode::AudioLooperNode(var params) :
	LooperNode(getTypeString(), params, AUDIO)
{
	numChannelsPerTrack = addIntParameter("Channel Per Track", "Number of channel to use for each track", 2, 1, 8);
	trackOutputMode = addEnumParameter("Output Mode", "How to output the channels");
	trackOutputMode->addOption("Mixed only", MIXED_ONLY)->addOption("Tracks only", SEPARATE_ONLY)->addOption("Mixed and tracks", ALL);

	setAudioInputs(numChannelsPerTrack->intValue());
	AudioManager::getInstance()->addAudioManagerListener(this);
}

AudioLooperNode::~AudioLooperNode()
{
	AudioManager::getInstance()->removeAudioManagerListener(this);
}

void AudioLooperNode::initInternal()
{
	LooperNode::initInternal();
	updateOutTracks(false);
	updateRingBuffer();
}

void AudioLooperNode::updateOutTracks(bool updateConfig)
{
	TrackOutputMode m = trackOutputMode->getValueDataAsEnum<TrackOutputMode>();
	StringArray s;

	if (m == MIXED_ONLY || m == ALL)
	{
		for (int i = 0; i < numChannelsPerTrack->intValue(); i++)
		{
			s.add("Mix " + String(i + 1));
		}
	}

	if (m == SEPARATE_ONLY || m == ALL)
	{
		for (int i = 0; i < numTracks->intValue(); i++)
		{
			s.add("Track " + String(i + 1));
		}
	}

	setAudioOutputs(s, updateConfig);
}

LooperTrack* AudioLooperNode::createLooperTrack(int index)
{
	return new AudioLooperTrack(this, index, numChannelsPerTrack->intValue());
}

void AudioLooperNode::updateRingBuffer()
{
	ScopedSuspender sp(processor);
	ringBuffer.reset(new RingBuffer<float>(numChannelsPerTrack->intValue(), getFadeNumSamples())); //fadeNumSamples WAS DOUBLED BEFORE, REMOVED BECAUSE I DON'T UNDERSTAND WHY. ->double to not have overlapping read and write (??)
	updateRetroRingBuffer();
}

void AudioLooperNode::updateRetroRingBuffer()
{
	ScopedSuspender sp(processor);

	RetroRecMode rm = retroRecMode->getValueDataAsEnum<RetroRecMode>();
	if (rm == RETRO_NONE)
	{
		retroRingBuffer.reset();
		return;
	}

	int maxNum = jmax(jmax(retroRecCount->intValue(), retroDoubleRecCount->intValue()), retroTripleRecCount->intValue());
	int numBeats = maxNum * getRetroBeatMultiplier();

	int numSamples = Transport::getInstance()->getSamplesForBeat(numBeats) + getFadeNumSamples();
	retroRingBuffer.reset(new RingBuffer<float>(numChannelsPerTrack->intValue(), numSamples));
}


int AudioLooperNode::getFadeNumSamples()
{
	return Transport::getInstance()->getBlockPerfectNumSamples(fadeTimeMS->intValue() * AudioManager::getInstance()->currentSampleRate / 1000, false);
}

void AudioLooperNode::audioSetupChanged()
{
	updateRingBuffer();
}


void AudioLooperNode::prepareToPlay(double sampleRate, int maximumExpectedSamplePerBlock)
{
	Node::prepareToPlay(sampleRate, maximumExpectedSamplePerBlock);
}

void AudioLooperNode::onContainerParameterChangedInternal(Parameter* p)
{
	LooperNode::onContainerParameterChangedInternal(p);

	if (p == trackOutputMode)
	{
		updateOutTracks();
		updateLooperTracks();
		currentTrackIndex->setRange(1, numTracks->intValue());
	}
	else if (p == numChannelsPerTrack)
	{
		setAudioInputs(numChannelsPerTrack->intValue());

		TrackOutputMode tom = trackOutputMode->getValueDataAsEnum<TrackOutputMode>();
		if (tom == MIXED_ONLY || tom == ALL) updateOutTracks();

		for (auto& cc : tracksCC.controllableContainers)
		{
			((AudioLooperTrack*)cc.get())->setNumChannels(numChannelsPerTrack->intValue());
		}

		updateRingBuffer();
	}

}

void AudioLooperNode::onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c)
{
	LooperNode::onControllableFeedbackUpdateInternal(cc, c);
	if (c == fadeTimeMS) updateRingBuffer();
	if (c == retroRecMode || c == retroRecCount || c == retroDoubleRecCount || c == retroTripleRecCount) updateRetroRingBuffer();
}

void AudioLooperNode::bpmChanged()
{
	LooperNode::bpmChanged();
	updateRetroRingBuffer();
}

void AudioLooperNode::playStateChanged(bool isPlaying, bool forceRestart)
{
	LooperNode::playStateChanged(isPlaying, forceRestart);
	updateRetroRingBuffer();
}

void AudioLooperNode::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
	if (ringBuffer->bufferSize == 0) return;

	TrackOutputMode tom = trackOutputMode->getValueDataAsEnum<TrackOutputMode>();
	MonitorMode mm = monitorMode->getValueDataAsEnum<MonitorMode>();
	int numMainChannels = tom == SEPARATE_ONLY ? 0 : numChannelsPerTrack->intValue();
	bool oneIsRecording = isOneTrackRecording();

	if (fadeTimeMS->intValue() > 0) ringBuffer->writeSamples(buffer, 0, jmin(buffer.getNumSamples(), ringBuffer->bufferSize));
	if (retroRingBuffer != nullptr) retroRingBuffer->writeSamples(buffer, 0, jmin(buffer.getNumSamples(), retroRingBuffer->bufferSize));

	AudioBuffer<float> tmpBuffer;
	tmpBuffer.makeCopyOf(buffer);

	if (mm == OFF || tom == SEPARATE_ONLY || (mm == RECORDING_ONLY && !oneIsRecording))
	{
		for (int i = 0; i < numMainChannels; i++) buffer.clear(i, 0, buffer.getNumSamples());
	}

	bool outputIfRecording = mm == RECORDING_ONLY || mm == ALWAYS;
	for (int i = 0; i < numTracks->intValue(); i++)
	{
		((AudioLooperTrack*)tracksCC.controllableContainers[i].get())->processBlock(tmpBuffer, buffer, numMainChannels, outputIfRecording);
	}
}
