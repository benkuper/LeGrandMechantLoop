/*
  ==============================================================================

	LooperNode.cpp
	Created: 15 Nov 2020 8:43:03am
	Author:  bkupe

  ==============================================================================
*/

#include "LooperNode.h"
#include "ui/LooperNodeViewUI.h"
#include "LooperTrack.h"

LooperProcessor::LooperProcessor(Node* node) :
	GenericNodeAudioProcessor(node),
	tracksCC("Tracks"),
	currentTrack(nullptr)
{
	nodeRef->viewUISize->setPoint(350, 220);

	const int defaultNumTracks = 8;

	recTrigger = addTrigger("Rec", "Record to the current track");
	clearCurrentTrigger = addTrigger("Clear","Clear the current track if not empty, otherwise clear the past one");
	clearAllTrigger = addTrigger("Clear All","Clear all the tracks");
	currentTrackIndex = addIntParameter("Current Track", "Index of the current track", 1, 1, defaultNumTracks);
	
	numTracks = addIntParameter("Track Count", "Number of tracks to use for this looper", defaultNumTracks, 1, 32);
	numChannelsPerTrack = addIntParameter("Channel Per Track", "Number of channel to use for each track", 1, 1, 8);
	autoNext = addBoolParameter("Auto Next", "If checked, will automatically select the next track to record", true);

	trackOutputMode = addEnumParameter("Output Mode", "How to output the channels");
	trackOutputMode->addOption("Mixed only", MIXED_ONLY)->addOption("Tracks only", SEPARATE_ONLY)->addOption("Mixed and tracks", ALL);

	monitorMode = addEnumParameter("Monitor Mode", "How to monitor");
	monitorMode->addOption("Off", OFF)->addOption("Always", ALWAYS)->addOption("Armed track", ARMED_TRACK)->addOption("When recording", RECORDING_ONLY);

	fadeTimeMS = addIntParameter("Fade Time", "Number of ms to fade between start and end of the loop", 200, 0, 2000);
	addChildControllableContainer(&tracksCC);

	node->setAudioInputs(numChannelsPerTrack->intValue());
	updateOutTracks();
	updateLooperTracks();

	updateRingBuffer();

	setCurrentTrack(getTrackForIndex(currentTrackIndex->intValue()-1));

	Transport::getInstance()->addTransportListener(this);
	AudioManager::getInstance()->addAudioManagerListener(this);
}

LooperProcessor::~LooperProcessor()
{
	Transport::getInstance()->removeTransportListener(this);
	AudioManager::getInstance()->removeAudioManagerListener(this);
}

void LooperProcessor::updateOutTracks()
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

void LooperProcessor::updateLooperTracks()
{
	bool shouldResume = !isSuspended();
	suspendProcessing(true);

	while (tracksCC.controllableContainers.size() > numTracks->intValue())
	{
		LooperTrack* t = getTrackForIndex(tracksCC.controllableContainers.size() - 1);
		if (t->isCurrent) setCurrentTrack(nullptr);

		tracksCC.removeChildControllableContainer(t);
	}

	Array<ControllableContainer*> tracksToAdd;
	for (int i = tracksCC.controllableContainers.size(); i < numTracks->intValue(); i++)
	{
		tracksCC.addChildControllableContainer(new LooperTrack(this, i, numChannelsPerTrack->intValue()), true);
	}

	if(shouldResume) suspendProcessing(false);
}

void LooperProcessor::updateRingBuffer()
{
	ringBuffer.reset(new RingBuffer<float>(numChannelsPerTrack->intValue(), getFadeNumSamples() * 2)); //double to not have overlapping read and write
}

void LooperProcessor::setCurrentTrack(LooperTrack* t)
{
	if (currentTrack == t) return;

	if (currentTrack != nullptr)
	{
		currentTrack->isCurrent->setValue(false);
	}

	currentTrack = t;

	if (currentTrack != nullptr)
	{
		currentTrack->isCurrent->setValue(true);
	}
}


void LooperProcessor::onContainerTriggerTriggered(Trigger* t)
{
	if (t == recTrigger)
	{
		if (currentTrack != nullptr)
		{
			if(currentTrack->isPlaying(true)) currentTrackIndex->setValue(currentTrackIndex->intValue()+1);
			
			currentTrack->playRecordTrigger->trigger();
			
			LooperTrack::TrackState s = currentTrack->trackState->getValueDataAsEnum<LooperTrack::TrackState>();
			
			if (s == LooperTrack::FINISH_RECORDING || currentTrack->isPlaying(true) && autoNext->boolValue())
			{
				currentTrackIndex->setValue(currentTrackIndex->intValue() + 1);
			}
		}
	}
	else if (t == clearCurrentTrigger)
	{
		if (currentTrack != nullptr)
		{
			if (autoNext->boolValue())
			{
				while (!currentTrack->hasContent(true) && currentTrackIndex->intValue() > 1)
				{
					currentTrackIndex->setValue(currentTrackIndex->intValue() - 1);
				}
			}
			currentTrack->clearTrigger->trigger();
		}
	}
	else if (t == clearAllTrigger)
	{
		for (auto& cc : tracksCC.controllableContainers)
		{
			((LooperTrack*)cc.get())->clearTrigger->trigger();
			currentTrackIndex->setValue(1);
		}
	}
}

void LooperProcessor::onContainerParameterChanged(Parameter* p)
{
	if (nodeRef.wasObjectDeleted()) return;

	if (p == numTracks || p == trackOutputMode)
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
			((LooperTrack*)cc.get())->setNumChannels(numChannelsPerTrack->intValue());
		}
	}
	else if (p == fadeTimeMS) updateRingBuffer();
	else if (p == currentTrackIndex)
	{
		setCurrentTrack(getTrackForIndex(currentTrackIndex->intValue()-1));
	}
}

void LooperProcessor::onControllableFeedbackUpdate(ControllableContainer* cc, Controllable* c)
{
	GenericNodeAudioProcessor::onControllableFeedbackUpdate(cc, c);
	if (LooperTrack* t = c->getParentAs<LooperTrack>())
	{
		if (c == t->trackState)
		{
			nodeRef->isNodePlaying->setValue(hasContent());
		}
	}
}


void LooperProcessor::beatChanged(bool isNewBar)
{
	for (auto& cc : tracksCC.controllableContainers) ((LooperTrack*)cc.get())->handleBeatChanged(isNewBar);
}

void LooperProcessor::playStateChanged(bool isPlaying)
{
	if (!isPlaying) for (auto& cc : tracksCC.controllableContainers) ((LooperTrack*)cc.get())->stopPlaying();
}

void LooperProcessor::audioSetupChanged()
{
	updateRingBuffer();
}

void LooperProcessor::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
	TrackOutputMode tom = trackOutputMode->getValueDataAsEnum<TrackOutputMode>();
	MonitorMode mm = monitorMode->getValueDataAsEnum<MonitorMode>();
	int numMainChannels = tom == SEPARATE_ONLY ? 0 : numChannelsPerTrack->intValue();
	bool oneIsRecording = isOneTrackRecording();

	if(fadeTimeMS->intValue() > 0) ringBuffer->writeSamples(buffer, 0, jmin(buffer.getNumSamples(), ringBuffer->bufferSize));

	AudioBuffer<float> tmpBuffer;
	tmpBuffer.makeCopyOf(buffer);

	if (mm == OFF || tom == SEPARATE_ONLY || (mm == RECORDING_ONLY && !oneIsRecording))
	{
		for (int i = 0; i < numMainChannels; i++) buffer.clear(i, 0, buffer.getNumSamples());
	}

	bool outputIfRecording = mm == RECORDING_ONLY || mm == ALWAYS;
	for (int i = 0; i < numTracks->intValue(); i++)
	{
		((LooperTrack*)tracksCC.controllableContainers[i].get())->processBlock(tmpBuffer, buffer, numMainChannels, outputIfRecording);
	}
}

bool LooperProcessor::hasContent()
{
	for (auto& cc : tracksCC.controllableContainers)
	{
		if (((LooperTrack*)cc.get())->hasContent(false)) return true;
	}
	return false;
}

bool LooperProcessor::isOneTrackRecording(bool includeWillRecord)
{
	for (auto& cc : tracksCC.controllableContainers)
	{
		if (((LooperTrack*)cc.get())->isRecording(includeWillRecord)) return true;
	}
	return false;
}

int LooperProcessor::getFadeNumSamples()
{
	return fadeTimeMS->intValue() * AudioManager::getInstance()->currentSampleRate / 1000;
}

LooperTrack* LooperProcessor::getTrackForIndex(int index)
{
	if (index >= tracksCC.controllableContainers.size()) return nullptr;
	return (LooperTrack*)tracksCC.controllableContainers[index].get();
}

NodeViewUI* LooperProcessor::createNodeViewUI()
{
	return new LooperNodeViewUI((GenericAudioNode<LooperProcessor>*)nodeRef.get());
}
