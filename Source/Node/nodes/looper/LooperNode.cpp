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

LooperNode::LooperNode(StringRef name, var params, LooperType looperType) :
	Node(name, params, looperType == AUDIO, looperType == AUDIO, false, looperType == AUDIO),
	looperType(looperType),
    currentTrack(nullptr),
    tracksCC("Tracks")
{
	saveAndLoadRecursiveData = true;

	const int defaultNumTracks = 8;

	quantization = addEnumParameter("Quantization", "The way to know when to stop recording. Default means getting the quantization from the Transport.\nBar/beat means it will stop the recording to fill an round number of bar/beat, even if you stop before. Free means it will stop instantly.");
	quantization->addOption("Default", Transport::DEFAULT)->addOption("Bar", Transport::BAR)->addOption("Beat", Transport::BEAT)->addOption("Free", Transport::FREE);
	
	freeFillMode = addEnumParameter("Fill Mode", "In free mode, allows to fill the buffer with empty data to complete a bar or a beat.", getQuantization() == Transport::FREE);
	freeFillMode->addOption("From Quantization", Transport::DEFAULT)->addOption("Bar", Transport::BAR)->addOption("Beat", Transport::BEAT)->addOption("Direct", Transport::FREE);

	recTrigger = addTrigger("Rec", "Record to the current track");
	clearCurrentTrigger = addTrigger("Clear","Clear the current track if not empty, otherwise clear the past one");
	currentTrackIndex = addIntParameter("Current Track", "Index of the current track", 1, 1, defaultNumTracks);
	
	playAllTrigger = addTrigger("Play All", "Stop all tracks");
	stopAllTrigger = addTrigger("Stop All", "Stop all tracks");
	clearAllTrigger = addTrigger("Clear All", "Clear all the tracks");

	numTracks = addIntParameter("Track Count", "Number of tracks to use for this looper", defaultNumTracks, 1, 32);
	
	monitorMode = addEnumParameter("Monitor Mode", "How to monitor");
	monitorMode->addOption("Always", ALWAYS)->addOption("Armed track", ARMED_TRACK)->addOption("When recording", RECORDING_ONLY)->addOption("Off", OFF);

	fadeTimeMS = addIntParameter("Fade Time", "Number of ms to fade between start and end of the loop", 20, 0, 2000);
	addChildControllableContainer(&tracksCC);


	Transport::getInstance()->addTransportListener(this);

	viewUISize->setPoint(360, 260);
}

LooperNode::~LooperNode()
{
	Transport::getInstance()->removeTransportListener(this);
}

void LooperNode::initInternal()
{
	updateLooperTracks();
	setCurrentTrack(getTrackForIndex(currentTrackIndex->intValue() - 1));
}

void LooperNode::updateLooperTracks()
{
	ScopedSuspender sp(processor);

	while (tracksCC.controllableContainers.size() > numTracks->intValue())
	{
		LooperTrack* t = getTrackForIndex(tracksCC.controllableContainers.size() - 1);
		if (t->isCurrent) setCurrentTrack(nullptr);

		tracksCC.removeChildControllableContainer(t);
	}

	Array<ControllableContainer*> tracksToAdd;
	for (int i = tracksCC.controllableContainers.size(); i < numTracks->intValue(); i++)
	{
		tracksCC.addChildControllableContainer(createLooperTrack(i), true);
	}
}

void LooperNode::setCurrentTrack(LooperTrack* t)
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


void LooperNode::onContainerTriggerTriggered(Trigger* t)
{
	Node::onContainerTriggerTriggered(t);

	if (t == recTrigger)
	{
		if (currentTrack != nullptr)
		{
			if(currentTrack->isPlaying(true)) currentTrackIndex->setValue(currentTrackIndex->intValue()+1);
			
			currentTrack->playRecordTrigger->trigger();
			
			LooperTrack::TrackState s = currentTrack->trackState->getValueDataAsEnum<LooperTrack::TrackState>();
			
			if ((s == LooperTrack::FINISH_RECORDING || currentTrack->isPlaying(true)))
			{
				currentTrackIndex->setValue(currentTrackIndex->intValue() + 1);
			}
		}
	}
	else if (t == clearCurrentTrigger)
	{
		if (currentTrack != nullptr)
		{
			while (!currentTrack->hasContent(true) && currentTrackIndex->intValue() > 1)
			{
				currentTrackIndex->setValue(currentTrackIndex->intValue() - 1);
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
	else if (t == playAllTrigger)
	{
		for (auto& cc : tracksCC.controllableContainers) ((LooperTrack*)cc.get())->playTrigger->trigger();
	}
	else if (t == stopAllTrigger)
	{
		for (auto& cc : tracksCC.controllableContainers) ((LooperTrack*)cc.get())->stopTrigger->trigger();
	}
}

void LooperNode::onContainerParameterChangedInternal(Parameter* p)
{
	Node::onContainerParameterChangedInternal(p);

	if (p == numTracks)
	{
		updateLooperTracks();
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

void LooperNode::onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c)
{
	Node::onControllableFeedbackUpdateInternal(cc, c);
	if (LooperTrack* t = c->getParentAs<LooperTrack>())
	{
		if (c == t->trackState)
		{
			isNodePlaying->setValue(hasContent());
		}
	}
}


void LooperNode::beatChanged(bool isNewBar)
{
	for (auto& cc : tracksCC.controllableContainers) ((LooperTrack*)cc.get())->handleBeatChanged(isNewBar);
}

void LooperNode::playStateChanged(bool isPlaying)
{
	if (!isPlaying) for (auto& cc : tracksCC.controllableContainers) ((LooperTrack*)cc.get())->stopPlaying();
}

bool LooperNode::hasContent()
{
	for (auto& cc : tracksCC.controllableContainers)
	{
		if (((LooperTrack*)cc.get())->hasContent(false)) return true;
	}
	return false;
}

bool LooperNode::isOneTrackRecording(bool includeWillRecord)
{
	for (auto& cc : tracksCC.controllableContainers)
	{
		if (((LooperTrack*)cc.get())->isRecording(includeWillRecord)) return true;
	}
	return false;
}

LooperTrack* LooperNode::getTrackForIndex(int index)
{
	if (index >= tracksCC.controllableContainers.size()) return nullptr;
	return (LooperTrack*)tracksCC.controllableContainers[index].get();
}

Transport::Quantization LooperNode::getQuantization()
{
	Transport::Quantization q = quantization->getValueDataAsEnum<Transport::Quantization>();
	if (q == Transport::DEFAULT) q = Transport::getInstance()->quantization->getValueDataAsEnum<Transport::Quantization>();
	return q;
}

Transport::Quantization LooperNode::getFreeFillMode()
{
	Transport::Quantization q = freeFillMode->getValueDataAsEnum<Transport::Quantization>();
	if (q == Transport::DEFAULT) q = getQuantization();
	return q;
}

BaseNodeViewUI* LooperNode::createViewUI()
{
	return new LooperNodeViewUI(this);
}
