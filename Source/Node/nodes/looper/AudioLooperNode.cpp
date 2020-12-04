/*
  ==============================================================================

    AudioLooperNode.cpp
    Created: 1 Dec 2020 11:16:14pm
    Author:  bkupe

  ==============================================================================
*/

#include "AudioLooperNode.h"
#include "AudioLooperTrack.h"
#include "Node/Node.h"

AudioLooperProcessor::AudioLooperProcessor(Node* n) :
    LooperProcessor(n, AUDIO)
{
	numChannelsPerTrack = addIntParameter("Channel Per Track", "Number of channel to use for each track", 2, 1, 8);
	trackOutputMode = addEnumParameter("Output Mode", "How to output the channels");
	trackOutputMode->addOption("Mixed only", MIXED_ONLY)->addOption("Tracks only", SEPARATE_ONLY)->addOption("Mixed and tracks", ALL);

	nodeRef->setAudioInputs(numChannelsPerTrack->intValue());
	updateLooperTracks();
	updateOutTracks();

	AudioManager::getInstance()->addAudioManagerListener(this);
}

AudioLooperProcessor::~AudioLooperProcessor()
{
	AudioManager::getInstance()->removeAudioManagerListener(this);
}

void AudioLooperProcessor::initInternal()
{
	LooperProcessor::initInternal();

	updateRingBuffer();
}

void AudioLooperProcessor::updateOutTracks()
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

	nodeRef->setAudioOutputs(s);
}

LooperTrack * AudioLooperProcessor::createLooperTrack(int index)
{
	return new AudioLooperTrack(this, index, numChannelsPerTrack->intValue());
}

void AudioLooperProcessor::updateRingBuffer()
{
	ringBuffer.reset(new RingBuffer<float>(numChannelsPerTrack->intValue(), getFadeNumSamples() * 2)); //double to not have overlapping read and write
}


int AudioLooperProcessor::getFadeNumSamples()
{
	return fadeTimeMS->intValue() * AudioManager::getInstance()->currentSampleRate / 1000;
}

void AudioLooperProcessor::audioSetupChanged()
{
	updateRingBuffer();
}

void AudioLooperProcessor::onContainerParameterChanged(Parameter* p)
{
	LooperProcessor::onContainerParameterChanged(p);

	if (nodeRef.wasObjectDeleted()) return;

	if (p == trackOutputMode)
	{
		updateOutTracks();
		updateLooperTracks();
		currentTrackIndex->setRange(1, numTracks->intValue());
	}
	else if (p == numChannelsPerTrack)
	{
		nodeRef->setAudioInputs(numChannelsPerTrack->intValue());

		TrackOutputMode tom = trackOutputMode->getValueDataAsEnum<TrackOutputMode>();
		if (tom == MIXED_ONLY || tom == ALL) updateOutTracks();

		for (auto& cc : tracksCC.controllableContainers)
		{
			((AudioLooperTrack*)cc.get())->setNumChannels(numChannelsPerTrack->intValue());
		}
	}
	else if (p == fadeTimeMS) updateRingBuffer();
}

void AudioLooperProcessor::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
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
