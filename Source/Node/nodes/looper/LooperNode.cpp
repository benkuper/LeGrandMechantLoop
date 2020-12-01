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

LooperProcessor::LooperProcessor(Node* node, LooperType looperType) :
	GenericNodeProcessor(node),
	looperType(looperType),
	tracksCC("Tracks"),
	currentTrack(nullptr)
{
	nodeRef->viewUISize->setPoint(350, 220);

	const int defaultNumTracks = 8;

	quantization = addEnumParameter("Quantization", "The way to know when to stop recording. Default means getting the quantization from the Transport.\nBar/beat means it will stop the recording to fill an round number of bar/beat, even if you stop before. Free means it will stop instantly.");
	quantization->addOption("Default", Transport::DEFAULT)->addOption("Bar", Transport::BAR)->addOption("Beat", Transport::BEAT)->addOption("Free", Transport::FREE);
	
	freeFillMode = addEnumParameter("Fill Mode", "In free mode, allows to fill the buffer with empty data to complete a bar or a beat.", getQuantization() == Transport::FREE);
	freeFillMode->addOption("From Quantization", Transport::DEFAULT)->addOption("Bar", Transport::BAR)->addOption("Beat", Transport::BEAT)->addOption("Direct", Transport::FREE);


	recTrigger = addTrigger("Rec", "Record to the current track");
	clearCurrentTrigger = addTrigger("Clear","Clear the current track if not empty, otherwise clear the past one");
	clearAllTrigger = addTrigger("Clear All","Clear all the tracks");
	currentTrackIndex = addIntParameter("Current Track", "Index of the current track", 1, 1, defaultNumTracks);
	
	numTracks = addIntParameter("Track Count", "Number of tracks to use for this looper", defaultNumTracks, 1, 32);
	autoNext = addBoolParameter("Auto Next", "If checked, will automatically select the next track to record", true);

	
	monitorMode = addEnumParameter("Monitor Mode", "How to monitor");
	monitorMode->addOption("Always", ALWAYS)->addOption("Armed track", ARMED_TRACK)->addOption("When recording", RECORDING_ONLY)->addOption("Off", OFF);

	fadeTimeMS = addIntParameter("Fade Time", "Number of ms to fade between start and end of the loop", 20, 0, 2000);
	addChildControllableContainer(&tracksCC);



	setCurrentTrack(getTrackForIndex(currentTrackIndex->intValue()-1));

	Transport::getInstance()->addTransportListener(this);
}

LooperProcessor::~LooperProcessor()
{
	Transport::getInstance()->removeTransportListener(this);
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

	if (p == numTracks)
	{
		currentTrackIndex->setRange(1, numTracks->intValue());
	}
	else if (p == currentTrackIndex)
	{
		setCurrentTrack(getTrackForIndex(currentTrackIndex->intValue()-1));
	}
	else if (p == quantization)
	{
		freeFillMode->setEnabled(getQuantization() == Transport::FREE);
	}
}

void LooperProcessor::onControllableFeedbackUpdate(ControllableContainer* cc, Controllable* c)
{
	GenericNodeProcessor::onControllableFeedbackUpdate(cc, c);
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

Transport::Quantization LooperProcessor::getQuantization()
{
	Transport::Quantization q = quantization->getValueDataAsEnum<Transport::Quantization>();
	if (q == Transport::DEFAULT) q = Transport::getInstance()->quantization->getValueDataAsEnum<Transport::Quantization>();
	return q;
}

Transport::Quantization LooperProcessor::getFreeFillMode()
{
	Transport::Quantization q = freeFillMode->getValueDataAsEnum<Transport::Quantization>();
	if (q == Transport::DEFAULT) q = getQuantization();
	return q;
}

NodeViewUI* LooperProcessor::createNodeViewUI()
{
	return new LooperNodeViewUI((GenericNode<LooperProcessor>*)nodeRef.get());
}
