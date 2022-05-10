#include "AudioLooperNode.h"
/*
  ==============================================================================

	AudioLooperNode.cpp
	Created: 1 Dec 2020 11:16:14pm
	Author:  bkupe

  ==============================================================================
*/

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
	updateOutTracks();
	updateRingBuffer();
}

void AudioLooperNode::updateOutTracks()
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

	setAudioOutputs(s);
}

LooperTrack* AudioLooperNode::createLooperTrack(int index)
{
	return new AudioLooperTrack(this, index, numChannelsPerTrack->intValue());
}

void AudioLooperNode::updateRingBuffer()
{
	ScopedSuspender sp(processor);
	ringBuffer.reset(new RingBuffer<float>(numChannelsPerTrack->intValue(), getFadeNumSamples() * 2)); //double to not have overlapping read and write
}


int AudioLooperNode::getFadeNumSamples()
{
	return fadeTimeMS->intValue() * AudioManager::getInstance()->currentSampleRate / 1000;
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
}

void AudioLooperNode::playStateChanged(bool isPlaying, bool forceRestart)
{
	LooperNode::playStateChanged(isPlaying, forceRestart);

	//if (!isPlaying)
	//{
	//	for (auto& cc : tracksCC.controllableContainers)
	//	{
	//		AudioLooperTrack* tc = (AudioLooperTrack*)cc.get();
	//		tc->antiClickFadeBeforePause = true;
	//	}
	//}
}

void AudioLooperNode::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
	TrackOutputMode tom = trackOutputMode->getValueDataAsEnum<TrackOutputMode>();
	MonitorMode mm = monitorMode->getValueDataAsEnum<MonitorMode>();
	int numMainChannels = tom == SEPARATE_ONLY ? 0 : numChannelsPerTrack->intValue();
	bool oneIsRecording = isOneTrackRecording();

	if (fadeTimeMS->intValue() > 0) ringBuffer->writeSamples(buffer, 0, jmin(buffer.getNumSamples(), ringBuffer->bufferSize));

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
