/*
  ==============================================================================

	LooperNode.cpp
	Created: 15 Nov 2020 8:43:03am
	Author:  bkupe

  ==============================================================================
*/

#include "Node/NodeIncludes.h"

LooperNode::LooperNode(StringRef name, var params, LooperType looperType) :
	Node(name, params, looperType == AUDIO, looperType == AUDIO, false, looperType == AUDIO, looperType == MIDI, looperType == MIDI),
	looperType(looperType),
	currentTrack(nullptr),
	trackParamsCC("Track Parameters"),
	recordCC("Recording"),
	saveLoadCC("Save and Load"),
	controlsCC("Controls"),
	tracksCC("Tracks")
{
	const int defaultNumTracks = 8;

	numTracks = trackParamsCC.addIntParameter("Track Count", "Number of tracks to use for this looper", defaultNumTracks, 1, 32);
	currentTrackIndex = trackParamsCC.addIntParameter("Current Track", "Index of the current track", 1, 1, defaultNumTracks);
	currentTrackIndex->isSavable = false;

	section = trackParamsCC.addIntParameter("Current Section", "The Section to set for the next recorded track (like A,B,C in a song). This allow to make changing structure and switch between sections during performance", 1, 1);
	playStopFadeMS = trackParamsCC.addIntParameter("Play Stop Fade", "Number of ms to fade when a track is about to stop", 10, 0, 2000);

	recordingState = recordCC.addEnumParameter("Recording State", "General feedback to know if the looper is recording at least one track");
	recordingState->addOption("Idle", LooperTrack::IDLE)->addOption("Will Record", LooperTrack::WILL_RECORD)->addOption("Recording", LooperTrack::RECORDING)->addOption("Finish Recording", LooperTrack::FINISH_RECORDING);
	recordingState->setControllableFeedbackOnly(true);

	quantization = recordCC.addEnumParameter("Quantization", "The way to know when to stop recording. Default means getting the quantization from the Transport.\nBar/beat means it will stop the recording to fill an round number of bar/beat, even if you stop before. Free means it will stop instantly.");
	quantization->addOption("Default", Transport::DEFAULT)->addOption("First Loop", Transport::FIRSTLOOP)->addOption("Bar", Transport::BAR)->addOption("Beat", Transport::BEAT)->addOption("Free", Transport::FREE);

	freeFillMode = recordCC.addEnumParameter("Fill Mode", "In free mode, allows to fill the buffer with empty data to complete a bar or a beat.", getQuantization() == Transport::FREE);
	freeFillMode->addOption("From Quantization", Transport::DEFAULT)->addOption("First Loop", Transport::FIRSTLOOP)->addOption("Bar", Transport::BAR)->addOption("Beat", Transport::BEAT)->addOption("Direct", Transport::FREE);

	doubleRecMode = recordCC.addEnumParameter("Double Rec Mode", "This decides what to do when hitting rec when the track is already preparing for recoring. This allow for nice behaviour such as automatically finish the record after one bar");
	doubleRecMode->addOption("First Loop Length", AUTO_STOP_FIRSTLOOP)->addOption("Record X bars", AUTO_STOP_BAR)->addOption("Record X beats", AUTO_STOP_BEAT)->addOption("Do nothing", NOTHING);
	doubleRecVal = recordCC.addIntParameter("Double Rec Value", "This is the number of bar or beats to record if Double Rec Mode is set to bar or beat.", 1, 1);

	tmpMuteMode = recordCC.addEnumParameter("Temp Mute Mode", "This is a convenient way of muting until next bar or next beat. This is trigger by the 'Controls > Temp Mute' trigger.");
	tmpMuteMode->addOption("Next FirstLoop", NEXT_FIRSTLOOP)->addOption("Next Bar", NEXT_BAR)->addOption("Next Beat", NEXT_BEAT);

	firstRecVolumeThreshold = recordCC.addFloatParameter("First Rec Threshold", "If enabled, when recording the first track when Transport not playing yet, this will wait for this threshold to be reached to actually start the recording", .3f, 0, 1, false);
	firstRecVolumeThreshold->canBeDisabledByUser = true;


	monitorMode = recordCC.addEnumParameter("Monitor Mode", "How to monitor");
	monitorMode->addOption("Always", ALWAYS)->addOption("Armed track", ARMED_TRACK)->addOption("When recording", RECORDING_ONLY)->addOption("Off", OFF);

	fadeTimeMS = recordCC.addIntParameter("Fade Time", "Number of ms to fade between start and end of the loop", 20, 0, 2000);

	saveLoadMode = saveLoadCC.addEnumParameter("Save Load Mode", "Save and load behaviour");
	saveLoadMode->addOption("None", NONE)->addOption("Auto", AUTO_SAVELOAD)->addOption("Auto no overwrite", AUTO_NO_OVERWRITE)->addOption("Load Only", LOAD_ONLY)->addOption("Save Only", SAVE_ONLY);

	sampleDirectory = saveLoadCC.addFileParameter("Sample Directory", "Directory where samples are stored. It will then look for 1.wav / 2.wav / 3.wav, etc.");
	sampleDirectory->directoryMode = true;

	saveSamplesTrigger = saveLoadCC.addTrigger("Save samples", "Save samples in the directory");
	loadSamplesTrigger = saveLoadCC.addTrigger("Load samples", "Load samples in the directory");
	clearSamplesTrigger = saveLoadCC.addTrigger("Clear samples", "Clear samples in the directory, and the buffer in the tracks");


	recTrigger = controlsCC.addTrigger("Rec", "Record to the current track");
	clearCurrentTrigger = controlsCC.addTrigger("Clear", "Clear the current track if not empty, otherwise clear the past one");
	playAllTrigger = controlsCC.addTrigger("Play All", "Stop all tracks");
	playCurrentSectionTrigger = controlsCC.addTrigger("Play Current Section", "This will play tracks in current section and stop all others");
	stopAllTrigger = controlsCC.addTrigger("Stop All", "Stop all tracks");
	clearSectionTrigger = controlsCC.addTrigger("Clear Section", "Clear all tracks in current section");
	clearOtherSectionsTrigger = controlsCC.addTrigger("Clear Other Sections", "Clear all tracks in other sections than the current one");
	clearAllTrigger = controlsCC.addTrigger("Clear All", "Clear all the tracks");
	tmpMuteAllTrigger = controlsCC.addTrigger("Temp Mute All", "This will mute all the tracks that are not already muted, and will unmute them depending on the Temp Mute Mode value");

	addChildControllableContainer(&trackParamsCC);
	addChildControllableContainer(&recordCC);
	addChildControllableContainer(&controlsCC);
	addChildControllableContainer(&saveLoadCC);
	addChildControllableContainer(&tracksCC);

	showGlobalControl = viewCC.addBoolParameter("Show Global Controls", "Show Global Controls", true);
	showTracks = viewCC.addBoolParameter("Show Tracks", "Shows the tracks in view", true);
	showTrackRec = viewCC.addBoolParameter("Show Tracks Rec", "Show Tracks Rec Control", true);
	showTrackStopClear = viewCC.addBoolParameter("Show Tracks Stop/Clear", "Show Tracks other controls", true);
	showTrackActives = viewCC.addBoolParameter("Show Tracks Active", "Show Tracks Active", true);
	showTrackGains = viewCC.addBoolParameter("Show Tracks Gain", "Show Tracks Gain", true);
	showTrackRMS = viewCC.addBoolParameter("Show Tracks RMS", "Show Tracks RMS", true);

	Transport::getInstance()->addTransportListener(this);
	viewUISize->setPoint(360, 290);

	Engine::mainEngine->addEngineListener(this);
}

LooperNode::~LooperNode()
{
	Transport::getInstance()->removeTransportListener(this);

	if (Engine* e = Engine::mainEngine) e->removeEngineListener(this);
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

	LooperTrack::lastManipulatedTrack = currentTrack;
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
			while (currentTrack->hasContent(false) && currentTrackIndex->intValue() < numTracks->intValue())
			{
				currentTrackIndex->setValue(currentTrackIndex->intValue() + 1);
			}

			LooperTrack* t = currentTrack; //may change during the process
			LooperTrack::TrackState s = t->trackState->getValueDataAsEnum<LooperTrack::TrackState>();

			if (s == LooperTrack::WILL_RECORD)
			{
				DoubleRecMode m = doubleRecMode->getValueDataAsEnum<DoubleRecMode>();
				if (m == AUTO_STOP_BAR || m == AUTO_STOP_BEAT || m == AUTO_STOP_FIRSTLOOP)
				{
					int numBeats = -1;
					if (m == AUTO_STOP_FIRSTLOOP) numBeats = Transport::getInstance()->firstLoopBeats->intValue();
					else numBeats = doubleRecVal->intValue() * (m == AUTO_STOP_BAR ? Transport::getInstance()->beatsPerBar->intValue() : 1);

					t->autoStopRecAfterBeats = numBeats;
				}
			}
			else
			{
				/*
				if (s == LooperTrack::FINISH_RECORDING)
				{
					while (currentTrack->isPlaying(true) && currentTrackIndex->intValue() < numTracks->intValue())
					{
						currentTrackIndex->setValue(currentTrackIndex->intValue() + 1);
					}
				}
				*/

				currentTrack->playRecordTrigger->trigger();
			}

			LooperTrack::TrackState newState = t->trackState->getValueDataAsEnum<LooperTrack::TrackState>();
			if (newState == LooperTrack::WILL_RECORD || newState == LooperTrack::RECORDING)
			{
				t->section->setValue(section->intValue());
			}


			if ((newState == LooperTrack::FINISH_RECORDING || t->isPlaying(true)))
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
	else if (c == clearSectionTrigger || c == clearOtherSectionsTrigger)
	{
		Array<LooperTrack*> tracks = c == clearSectionTrigger ? getTracksForSection(section->intValue()) : getTracksExceptSection(section->intValue());
		for (auto& t : tracks) t->clearTrigger->trigger();
		setCurrentTrackToFirstEmpty();
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
		for (auto& cc : tracksCC.controllableContainers)
		{
			((LooperTrack*)cc.get())->playTrigger->trigger();
		}
	}
	else if (c == playCurrentSectionTrigger)
	{
		for (auto& cc : tracksCC.controllableContainers)
		{
			LooperTrack* t = (LooperTrack*)cc.get();
			if (t->section->intValue() == section->intValue()) t->playTrigger->trigger();
			else t->stopTrigger->trigger();
		}
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
	else if (c == doubleRecMode)
	{
		doubleRecVal->setEnabled(doubleRecMode->getValueDataAsEnum<DoubleRecMode>() != AUTO_STOP_FIRSTLOOP);
	}
	else if (c == sampleDirectory)
	{
		SaveLoadMode m = saveLoadMode->getValueDataAsEnum<SaveLoadMode>();
		if (m != SAVE_ONLY) loadSamples();
	}
	else if (c == saveSamplesTrigger)
	{
		SaveLoadMode m = saveLoadMode->getValueDataAsEnum<SaveLoadMode>();
		if (m != LOAD_ONLY) saveSamples();
	}
	else if (c == loadSamplesTrigger)
	{
		SaveLoadMode m = saveLoadMode->getValueDataAsEnum<SaveLoadMode>();
		if (m != SAVE_ONLY) loadSamples();
	}
	else if (c == clearSamplesTrigger)
	{
		clearSamples();
	}


	else if (LooperTrack* t = c->getParentAs<LooperTrack>())
	{
		if (c == t->trackState)
		{
			isNodePlaying->setValue(hasContent(false));

			bool recordOrWillRecord = isOneTrackRecording(true);
			bool isActuallyRecording = isOneTrackRecording(false);
			LooperTrack::TrackState s = t->trackState->getValueDataAsEnum<LooperTrack::TrackState>();

			LooperTrack::TrackState ts;

			if (isActuallyRecording && s == LooperTrack::FINISH_RECORDING) ts = LooperTrack::FINISH_RECORDING;
			else if (isActuallyRecording) ts = LooperTrack::RECORDING;
			else if (recordOrWillRecord) ts = LooperTrack::WILL_RECORD;
			else ts = LooperTrack::IDLE;

			recordingState->setValueWithData(ts);
		}
	}
}

void LooperNode::bpmChanged()
{
	prepareToPlay(Transport::getInstance()->sampleRate, Transport::getInstance()->blockSize);

	ScopedSuspender sp(processor);
	for (int i = 0; i < numTracks->intValue(); i++)
	{
		((AudioLooperTrack*)tracksCC.controllableContainers[i].get())->updateStretch();
	}

}

void LooperNode::beatChanged(bool isNewBar, bool isFirstLoop)
{
	for (auto& cc : tracksCC.controllableContainers) ((LooperTrack*)cc.get())->handleBeatChanged(isNewBar, isFirstLoop);

	TempMuteMode m = tmpMuteMode->getValueDataAsEnum<TempMuteMode>();
	if (m == NEXT_BEAT || (m == NEXT_BAR && isNewBar) || (m == NEXT_FIRSTLOOP && isFirstLoop))
	{
		for (auto& t : tmpMuteTracks) t->active->setValue(true);
		tmpMuteTracks.clear();
	}


}

void LooperNode::playStateChanged(bool isPlaying, bool forceRestart)
{
	if (isPlaying && forceRestart)
	{
		for (auto& cc : tracksCC.controllableContainers)
		{
			LooperTrack* tc = (LooperTrack*)cc.get();
			if (tc->isPlaying(true) && tc->playQuantization != Transport::FREE) tc->startPlaying();
		}
	}
	//if (!isPlaying)
	//{
	//	for (auto& cc : tracksCC.controllableContainers)
	//	{
	//		LooperTrack* tc = (LooperTrack*)cc.get();
	//	//	if(tc->playQuantization != Transport::FREE) tc->stopPlaying();
	//	}
	//}
}


void LooperNode::loadSamples()
{
	File dir = sampleDirectory->getFile();
	if (!dir.exists()) return;
	for (int i = 0; i < numTracks->intValue(); i++) if (LooperTrack* t = getTrackForIndex(i)) t->loadSampleFile(dir);
}

void LooperNode::saveSamples()
{
	clearSamples();

	File dir = sampleDirectory->getFile();
	if (!dir.exists()) return;
	for (int i = 0; i < numTracks->intValue(); i++) if (LooperTrack* t = getTrackForIndex(i)) t->saveSampleFile(dir);
}

void LooperNode::clearSamples()
{
	File dir = sampleDirectory->getFile();
	if (dir.exists()) dir.deleteFile();
	dir.createDirectory();
}

bool LooperNode::hasContent(bool includeFreeTracks)
{
	for (auto& cc : tracksCC.controllableContainers)
	{
		LooperTrack* tc = (LooperTrack*)cc.get();
		if (!includeFreeTracks && tc->playQuantization == Transport::FREE) continue;
		if (tc->hasContent(false)) return true;
	}
	return false;
}

bool LooperNode::areAllTrackedStopped(bool includeFreeTracks)
{
	for (auto& cc : tracksCC.controllableContainers)
	{
		LooperTrack* tc = (LooperTrack*)cc.get();
		if (!includeFreeTracks && tc->playQuantization == Transport::FREE) continue;
		if (tc->isPlaying(true) || tc->isRecording(true)) return false;
	}

	return true;
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

LooperTrack* LooperNode::getFirstEmptyTrack()
{
	for (auto& cc : tracksCC.controllableContainers)
	{
		LooperTrack* t = (LooperTrack*)cc.get();
		if (!t->hasContent(true)) return t;
	}
	return nullptr;
}

void LooperNode::setCurrentTrackToFirstEmpty()
{
	LooperTrack* t = getFirstEmptyTrack();
	if (t == nullptr) currentTrackIndex->setValue(1);
	else currentTrackIndex->setValue(tracksCC.controllableContainers.indexOf(t) + 1);
}

Array<LooperTrack*> LooperNode::getTracksForSection(int targetSection)
{
	Array<LooperTrack*> result;
	for (auto& cc : tracksCC.controllableContainers)
	{
		LooperTrack* t = (LooperTrack*)cc.get();
		if (t->section->intValue() == targetSection) result.add(t);
	}

	return result;
}

Array<LooperTrack*> LooperNode::getTracksExceptSection(int targetSection)
{
	Array<LooperTrack*> result;
	for (auto& cc : tracksCC.controllableContainers)
	{
		LooperTrack* t = (LooperTrack*)cc.get();
		if (t->section->intValue() != targetSection) result.add(t);
	}

	return result;
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

var LooperNode::getJSONData()
{
	var data = Node::getJSONData();
	data.getDynamicObject()->setProperty(controlsCC.shortName, controlsCC.getJSONData());
	data.getDynamicObject()->setProperty(trackParamsCC.shortName, trackParamsCC.getJSONData());
	data.getDynamicObject()->setProperty(recordCC.shortName, recordCC.getJSONData());
	data.getDynamicObject()->setProperty(saveLoadCC.shortName, saveLoadCC.getJSONData());
	var tracksData;
	for (auto& t : tracksCC.controllableContainers) tracksData.append(t->getJSONData());
	data.getDynamicObject()->setProperty("tracks", tracksData);
	return data;
}

void LooperNode::loadJSONDataItemInternal(var data)
{
	Node::loadJSONDataItemInternal(data);
	controlsCC.loadJSONData(data.getProperty(controlsCC.shortName, var()));
	trackParamsCC.loadJSONData(data.getProperty(trackParamsCC.shortName, var()));
	recordCC.loadJSONData(data.getProperty(recordCC.shortName, var()));
	saveLoadCC.loadJSONData(data.getProperty(saveLoadCC.shortName, var()));

	updateLooperTracks();

	var tracksData = data.getProperty("tracks", var());
	for (int i = 0; i < tracksData.size(); i++)
	{
		if (LooperTrack* tc = getTrackForIndex(i)) tc->loadJSONData(tracksData[i]);
	}
}

void LooperNode::fileSaved(bool)
{
	SaveLoadMode m = saveLoadMode->getValueDataAsEnum<SaveLoadMode>();
	if (m == AUTO_SAVELOAD || m == AUTO_NO_OVERWRITE) saveSamples();
}

void LooperNode::endLoadFile()
{
	SaveLoadMode m = saveLoadMode->getValueDataAsEnum<SaveLoadMode>();
	if (m == AUTO_SAVELOAD || m == AUTO_NO_OVERWRITE) loadSamples();
}

BaseNodeViewUI* LooperNode::createViewUI()
{
	return new LooperNodeViewUI(this);
}
