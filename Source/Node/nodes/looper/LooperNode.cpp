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
	trackParamsCC("Track Parameters"),
	recordCC("Recording"),
	controlsCC("Controls"),
    tracksCC("Tracks")
{
	saveAndLoadRecursiveData = true;

	const int defaultNumTracks = 8;

	numTracks = trackParamsCC.addIntParameter("Track Count", "Number of tracks to use for this looper", defaultNumTracks, 1, 32);
	currentTrackIndex = trackParamsCC.addIntParameter("Current Track", "Index of the current track", 1, 1, defaultNumTracks);
	currentTrackIndex->isSavable = false;
	
	quantization = recordCC.addEnumParameter("Quantization", "The way to know when to stop recording. Default means getting the quantization from the Transport.\nBar/beat means it will stop the recording to fill an round number of bar/beat, even if you stop before. Free means it will stop instantly.");
	quantization->addOption("Default", Transport::DEFAULT)->addOption("Bar", Transport::BAR)->addOption("Beat", Transport::BEAT)->addOption("Free", Transport::FREE);
	
	freeFillMode = recordCC.addEnumParameter("Fill Mode", "In free mode, allows to fill the buffer with empty data to complete a bar or a beat.", getQuantization() == Transport::FREE);
	freeFillMode->addOption("From Quantization", Transport::DEFAULT)->addOption("Bar", Transport::BAR)->addOption("Beat", Transport::BEAT)->addOption("Direct", Transport::FREE);

	doubleRecMode = recordCC.addEnumParameter("Double Rec Mode", "This decides what to do when hitting rec when the track is already preparing for recoring. This allow for nice behaviour such as automatically finish the record after one bar");
	doubleRecMode->addOption("Record X bars", AUTO_STOP_BAR)->addOption("Record X beats", AUTO_STOP_BEAT)->addOption("Do nothing", NOTHING);
	doubleRecVal = recordCC.addIntParameter("Double Rec Value", "This is the number of bar or beats to record if Double Rec Mode is set to bar or beat.", 1, 1);
	
	tmpMuteMode = recordCC.addEnumParameter("Temp Mute Mode", "This is a convenient way of muting until next bar or next beat. This is trigger by the 'Controls > Temp Mute' trigger.");
	tmpMuteMode->addOption("Next Bar", NEXT_BAR)->addOption("Next Beat", NEXT_BEAT);

	firstRecVolumeThreshold = recordCC.addFloatParameter("First Rec Threshold", "If enabled, when recording the first track when Transport not playing yet, this will wait for this threshold to be reached to actually start the recording", .3f, 0, 1, false);
	firstRecVolumeThreshold->canBeDisabledByUser = true;

	monitorMode = recordCC.addEnumParameter("Monitor Mode", "How to monitor");
	monitorMode->addOption("Always", ALWAYS)->addOption("Armed track", ARMED_TRACK)->addOption("When recording", RECORDING_ONLY)->addOption("Off", OFF);

	fadeTimeMS = recordCC.addIntParameter("Fade Time", "Number of ms to fade between start and end of the loop", 20, 0, 2000);

	recTrigger = controlsCC.addTrigger("Rec", "Record to the current track");
	clearCurrentTrigger = controlsCC.addTrigger("Clear","Clear the current track if not empty, otherwise clear the past one");
	playAllTrigger = controlsCC.addTrigger("Play All", "Stop all tracks");
	stopAllTrigger = controlsCC.addTrigger("Stop All", "Stop all tracks");
	clearAllTrigger = controlsCC.addTrigger("Clear All", "Clear all the tracks");
	tmpMuteAllTrigger = controlsCC.addTrigger("Temp Mute All", "This will mute all the tracks that are not already muted, and will unmute them depending on the Temp Mute Mode value");

	addChildControllableContainer(&trackParamsCC);
	addChildControllableContainer(&recordCC);
	addChildControllableContainer(&controlsCC);

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

void LooperNode::onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c)
{
	Node::onControllableFeedbackUpdateInternal(cc, c);

	if (c == numTracks)
	{
		updateLooperTracks();
		currentTrackIndex->setRange(1, numTracks->intValue());
	}
	else if (c == currentTrackIndex)
	{
		setCurrentTrack(getTrackForIndex(currentTrackIndex->intValue() - 1));
	}
	else if (c == quantization)
	{
		freeFillMode->setEnabled(getQuantization() == Transport::FREE);
	}

	else if (c == recTrigger)
	{
		if (currentTrack != nullptr)
		{
			while (currentTrack->isPlaying(true) && currentTrackIndex->intValue() < numTracks->intValue())
			{
				currentTrackIndex->setValue(currentTrackIndex->intValue() + 1);
			}

			LooperTrack::TrackState s = currentTrack->trackState->getValueDataAsEnum<LooperTrack::TrackState>();
			
			if (s == LooperTrack::WILL_RECORD)
			{
				DoubleRecMode m = doubleRecMode->getValueDataAsEnum<DoubleRecMode>();
				if (m == AUTO_STOP_BAR || m == AUTO_STOP_BEAT)
				{
					int numBeats = doubleRecVal->intValue() * (m == AUTO_STOP_BAR ? Transport::getInstance()->beatsPerBar->intValue() : 1);
					currentTrack->autoStopRecAfterBeats = numBeats;
				}
			}
			else
			{
				currentTrack->playRecordTrigger->trigger();
			}


			if ((s == LooperTrack::FINISH_RECORDING || currentTrack->isPlaying(true)))
			{
				currentTrackIndex->setValue(currentTrackIndex->intValue() + 1);
			}
		}
	}
	else if (c == clearCurrentTrigger)
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
	else if (c == clearAllTrigger)
	{
		for (auto& cc : tracksCC.controllableContainers)
		{
			((LooperTrack*)cc.get())->clearTrigger->trigger();
		}

		currentTrackIndex->setValue(1);
		if (outControl != nullptr) outControl->resetGainAndActive();
	}
	else if (c == playAllTrigger)
	{
		for (auto& cc : tracksCC.controllableContainers) ((LooperTrack*)cc.get())->playTrigger->trigger();
	}
	else if (c == stopAllTrigger)
	{
		for (auto& cc : tracksCC.controllableContainers) ((LooperTrack*)cc.get())->stopTrigger->trigger();
	}
	else if (c == tmpMuteAllTrigger)
	{
		for (auto& cc : tracksCC.controllableContainers)
		{
			LooperTrack* t = (LooperTrack*)cc.get();
			if (t->active->boolValue())
			{
				tmpMuteTracks.add(t);
				t->active->setValue(false);
			}
		}
	}


	else if (LooperTrack* t = c->getParentAs<LooperTrack>())
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
	
	TempMuteMode m = tmpMuteMode->getValueDataAsEnum<TempMuteMode>();
	if (m == NEXT_BEAT || (m == NEXT_BAR && isNewBar))
	{
		for (auto& t : tmpMuteTracks) t->active->setValue(true);
		tmpMuteTracks.clear();
	}
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
