/*
  ==============================================================================

	SamplerNode.cpp
	Created: 14 Apr 2021 8:40:43am
	Author:  bkupe

  ==============================================================================
*/

#include "Node/NodeIncludes.h"

SamplerNode::SamplerNode(var params) :
	Node(getTypeStringStatic(), params, true, true, false, true, true, true),
	recordingNote(-1),
	lastRecordedNote(-1),
	lastPlayedNote(-1),
	recordedSamples(0),
	noteStatesCC("Notes"),
	viewStartKey(20),
	isUpdatingLibrary(false),
	controlsCC("Controls"),
	playCC("Play"),
	recordCC("Record"),
	adsrCC("ADSR"),
	libraryCC("Library"),
	curBankIndex(1)
{
	controlsCC.includeTriggersInSaveLoad = true;
	saveAndLoadRecursiveData = true;

	numChannels = playCC.addIntParameter("Channels", "Number of channels", 1, 1);
	monitor = playCC.addBoolParameter("Monitor", "Monitor input audio", true);
	playMode = playCC.addEnumParameter("Play Mode", "How samples are treated");
	playMode->addOption("Hit (Loop)", HIT_LOOP)->addOption("Hit (One shot)", HIT_ONESHOT)->addOption("Peek (Continuous)", PEEK)->addOption("Keep (Hold)", KEEP);

	hitMode = playCC.addEnumParameter("Hit Mode", "How playing a sample is handled.\nPiano is normal playing, releasing a key stops the sample.\nHit is starting the sample but releasing doesn't affect (only works in One Shot play mode) Full will not take in account playing again the note if the sample is still playing. Reset will allow to replay from start.\nToggle is alternating key presses to start/stop instead of using key release");
	hitMode->addOption("Piano", PIANO)->addOption("Hit (Full Sample)", HIT_FULL)->addOption("Hit (Reset)", HIT_RESET)->addOption("Toggle", TOGGLE);

	autoKeyLiveMode = playCC.addBoolParameter("Auto Key", "If checked, hitting an unrecorded note will play it repitched from the closest found", false);
	autoKeyFadeTimeMS = playCC.addIntParameter("Auto Key Fade", "Fade for anticlick when repitching auto keys, in milliseconds", 50);

	addChildControllableContainer(&playCC, false, 0);


	clearMode = controlsCC.addBoolParameter("Clear Mode", "When checked, recorded or played notes are cleared depending on clearing mode", false);
	clearLastMode = controlsCC.addEnumParameter("Clear Last Mode", "The way to clear when clear last is triggered");
	clearLastMode->addOption("Last Played", LAST_PLAYED)->addOption("Last Recorded", LAST_RECORDED);

	clearLastRecordedTrigger = controlsCC.addTrigger("Clear", "Clear Last recorded or played note");
	clearAllNotesTrigger = controlsCC.addTrigger("Clear All Notes", "Clear all recorded notes");

	startAutoKey = controlsCC.addIntParameter("Start Auto Key", "The pitch to start auto key computation from", 0, 0, 127);
	endAutoKey = controlsCC.addIntParameter("End Auto Key", "The pitch to end auto key computation from", 127, 0, 127);
	computeAutoKeysTrigger = controlsCC.addTrigger("Compute Auto Keys", "If checked, the auto keys will be computed from the recorded notes", true);

	addChildControllableContainer(&controlsCC, false, 1);

	fadeTimeMS = recordCC.addIntParameter("Fade Time", "Time to fade between start and end of recording, in milliseconds", 20);
	isRecording = recordCC.addBoolParameter("Is Recording", "Is recording a note ?", false);
	isRecording->setControllableFeedbackOnly(true);

	addChildControllableContainer(&recordCC, false, 2);

	attack = adsrCC.addFloatParameter("Attack", "Time of the attack in seconds", .01f, 0);
	attack->defaultUI = FloatParameter::TIME;

	attackCurve = adsrCC.addFloatParameter("Attack Curve", "Bend the attack", 0.06f, 0, 0.1f);

	decay = adsrCC.addFloatParameter("Decay", "Time of decay in seconds", .2f, 0);
	decay->defaultUI = FloatParameter::TIME;

	sustain = new DecibelFloatParameter("Sustain", "Sustain dB");
	adsrCC.addParameter(sustain);

	release = adsrCC.addFloatParameter("Release", "Time of the release in seconds", .2f, 0);
	release->defaultUI = FloatParameter::TIME;

	releaseCurve = adsrCC.addFloatParameter("Release Curve", "Bend the release", 0.001f, 0, 0.1f);

	addChildControllableContainer(&adsrCC, false, 3);

	libraryFolder = libraryCC.addFileParameter("Lirbary Folder", "To export and import");
	libraryFolder->directoryMode = true;

	currentLibrary = libraryCC.addEnumParameter("Current Library", "Select a specific sub folder or the none for the root folder");
	currentBank = libraryCC.addIntParameter("Current Bank", "Index of the bank in the library folder, starting with 1. This will load a folder with a name starting with this number", 1, 1, 100);

	nextBank = libraryCC.addTrigger("Next Bank", "Goes to next bank");
	prevBank = libraryCC.addTrigger("Previous Bank", "Goes to previous bank");

	bankDescription = libraryCC.addStringParameter("Bank Description", "Description for this bank if available", "");

	autoLoadBank = libraryCC.addBoolParameter("Auto Load Bank", "If checked, the bank will be loaded when the current bank is changed", true);
	loadBankTrigger = libraryCC.addTrigger("Load Bank", "Load all samples from the current bank");
	saveBankTrigger = libraryCC.addTrigger("Save Bank", "Export all samples at once to this bank's folder");
	showFolderTrigger = libraryCC.addTrigger("Show Folder", "Show the folder in explorer");

	addChildControllableContainer(&libraryCC, false, 4);

	keyboardState.addListener(this);

	int sr = processor->getSampleRate();

	for (int i = 0; i < 128; i++)
	{
		SamplerNote* sn = new SamplerNote();
		sn->buffer.clear();
		sn->adsr.setAttackRate(attack->floatValue() * sr);
		sn->adsr.setTargetRatioA(attackCurve->floatValue());
		sn->adsr.setDecayRate(decay->floatValue() * sr);
		sn->adsr.setSustainLevel(sustain->gain);
		sn->adsr.setReleaseRate(release->floatValue() * sr);
		sn->adsr.setTargetRatioDR(releaseCurve->floatValue());

		sn->state = noteStatesCC.addEnumParameter(MidiMessage::getMidiNoteName(i, true, true, 3), "State for this note");
		sn->state->addOption("Empty", EMPTY)->addOption("Recording", RECORDING)->addOption("Filled", FILLED)->addOption("Processing", PROCESSING)->addOption("Playing", PLAYING);
		sn->state->setControllableFeedbackOnly(true);

		sn->oneShotted = false;

		samplerNotes.add(sn);
	}

	updateBuffers();
	setAudioInputs(numChannels->intValue());
	setAudioOutputs(numChannels->intValue());

	addChildControllableContainer(&noteStatesCC);
	noteStatesCC.hideInEditor = true;

	setMIDIIO(true, true);
}

SamplerNode::~SamplerNode()
{
	samplerNotes.clear();
}

void SamplerNode::clearNote(int note)
{
	if (note == -1) return;
	ScopedSuspender sp(processor);
	samplerNotes[note]->adsr.reset();
	samplerNotes[note]->buffer.setSize(0, 0);
	samplerNotes[note]->state->setValueWithData(EMPTY);

}

void SamplerNode::clearAllNotes()
{
	ScopedSuspender sp(processor);
	for (auto& n : samplerNotes)
	{
		n->adsr.reset();
		n->buffer.setSize(0, 0);
		n->state->setValueWithData(EMPTY);
	}
}

void SamplerNode::resetAllNotes()
{
	ScopedSuspender sp(processor);
	for (auto& n : samplerNotes)
	{
		n->reset();
	}
}

void SamplerNode::computeAutoKeys()
{
	//ScopedSuspender sp(processor);

	for (int i = 0; i < samplerNotes.size(); i++)
	{
		NoteState ns = samplerNotes[i]->state->getValueDataAsEnum<NoteState>();
		if (ns == PROCESSING || samplerNotes[i]->isProxyNote()) clearNote(i);
	}

	int startKey = startAutoKey->intValue();
	int endKey = endAutoKey->intValue();

	for (int i = startKey; i <= endKey; i++)
	{
		SamplerNote* n = samplerNotes[i];
		if (n == nullptr) return;
		NoteState ns = n->state->getValueDataAsEnum<NoteState>();
		if (ns != EMPTY) continue;

		int closestNote = -1;
		{
			for (int j = 1; j <= 127; j++)
			{
				int lowI = i - j;
				int highI = i + j;
				if (lowI >= 0 && samplerNotes[lowI]->hasContent())
				{
					closestNote = lowI;
					break;
				}
				else if (highI < 128 && samplerNotes[highI]->hasContent())
				{
					closestNote = highI;
					break;
				}
			}
		}

		if (closestNote != -1)
		{
			double shift = MidiMessage::getMidiNoteInHertz(i) / MidiMessage::getMidiNoteInHertz(closestNote);
			n->computeAutoKey(samplerNotes[closestNote], shift, getFadeNumSamples(autoKeyFadeTimeMS->intValue()));
		}
	}
}


void SamplerNode::updateBuffers()
{
	ScopedSuspender sp(processor);
	updateRingBuffer();
	for (auto& n : samplerNotes)  n->buffer.setSize(getNumAudioInputs(), n->buffer.getNumSamples(), false, true);
}

void SamplerNode::updateRingBuffer()
{
	ScopedSuspender sp(processor);
	ringBuffer.reset(new RingBuffer<float>(getNumAudioInputs(), getFadeNumSamples(fadeTimeMS->intValue()) * 2)); //double to not have overlapping read and write

}

void SamplerNode::startRecording(int note)
{
	if (recordingNote != -1) return;

	//ScopedSuspender sp(processor);

	GenericScopedLock<SpinLock> lock(recLock);

	isRecording->setValue(true);
	recordingNote = note;

	int recNumSamples = processor->getSampleRate() * 60; // 1 min rec samples

	SamplerNote* samplerNote = samplerNotes[note];
	samplerNote->state->setValueWithData(RECORDING);

	samplerNote->buffer.setSize(getNumAudioInputs(), recNumSamples, false, true);

	int fadeNumSamples = getFadeNumSamples(fadeTimeMS->intValue());
	if (fadeNumSamples > 0)
	{
		preRecBuffer.setSize(samplerNote->buffer.getNumChannels(), fadeNumSamples, false, true);
		ringBuffer->readSamples(preRecBuffer, fadeNumSamples);
	}

	recordedSamples = 0;
}

void SamplerNode::stopRecording()
{
	if (recordingNote == -1) return;

	//ScopedSuspender sp(processor);

	GenericScopedTryLock<SpinLock> lock(recLock);

	SamplerNote* samplerNote = samplerNotes[recordingNote];
	recordedSamples = recordedSamples - (recordedSamples % processor->getBlockSize()); //fit to blockSize

	if (recordedSamples > 0)
	{
		//fade with ring buffer using looper fadeTimeMS
		int fadeNumSamples = getFadeNumSamples(fadeTimeMS->intValue());
		samplerNote->buffer.setSize(samplerNote->buffer.getNumChannels(), recordedSamples, true);

		if (fadeNumSamples > 0)
		{
			int cropFadeNumSamples = jmin(recordedSamples, fadeNumSamples);
			int bufferStartSample = recordedSamples - cropFadeNumSamples;

			int preRecStartSample = preRecBuffer.getNumSamples() - cropFadeNumSamples;

			samplerNote->buffer.applyGainRamp(bufferStartSample, cropFadeNumSamples, 1, 0);

			for (int i = 0; i < samplerNote->buffer.getNumChannels(); i++)
			{
				samplerNote->buffer.addFromWithRamp(i, bufferStartSample, preRecBuffer.getReadPointer(i, preRecStartSample), cropFadeNumSamples, 0, 1);
			}

			preRecBuffer.clear();
		}

		samplerNote->state->setValueWithData(FILLED);
	}
	else
	{
		samplerNote->state->setValueWithData(EMPTY);
	}

	lastRecordedNote = recordingNote;
	lastPlayedNote = recordingNote;
	recordingNote = -1;
	isRecording->setValue(false);
}


void SamplerNode::onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c)
{
	Node::onControllableFeedbackUpdateInternal(cc, c);

	if (c == numChannels)
	{
		setAudioInputs(numChannels->intValue());
		setAudioOutputs(numChannels->intValue());
		updateBuffers();
	}
	if (c == attack || c == decay || c == sustain || c == release || c == attackCurve || c == releaseCurve)
	{
		int sr = processor->getSampleRate();
		for (int i = 0; i < 128; i++)
		{
			if (c == attack) samplerNotes[i]->adsr.setAttackRate(attack->floatValue() * sr);
			else if (c == attackCurve) samplerNotes[i]->adsr.setTargetRatioA(attackCurve->floatValue());
			else if (c == decay) samplerNotes[i]->adsr.setDecayRate(decay->floatValue() * sr);
			else if (c == sustain) samplerNotes[i]->adsr.setSustainLevel(sustain->gain);
			else if (c == release) samplerNotes[i]->adsr.setReleaseRate(release->floatValue() * sr);
			else if (c == releaseCurve) samplerNotes[i]->adsr.setTargetRatioDR(releaseCurve->floatValue());
		}

	}
	else if (c == fadeTimeMS)
	{
		updateRingBuffer();
	}
	else if (c == libraryFolder)
	{
		updateLibraries();
	}
	else if (c == currentLibrary)
	{
		if (!isUpdatingLibrary)
		{
			String b = currentLibrary->getValueData().toString();
			if (b == "new")
			{
				//create a window with name parameter to create a new folder
				AlertWindow* window = new AlertWindow("Add a Bank", "Add a new bank", AlertWindow::AlertIconType::NoIcon);
				window->addTextEditor("bank", libraryFolder->getFile().getNonexistentChildFile("Library", "", true).getFileName(), "Library Name");
				window->addButton("OK", 1, KeyPress(KeyPress::returnKey));
				window->addButton("Cancel", 0, KeyPress(KeyPress::escapeKey));

				window->enterModalState(true, ModalCallbackFunction::create([window, this](int result)
					{

						if (result)
						{
							String bankName = window->getTextEditorContents("bank");
							File f = libraryFolder->getFile().getChildFile(bankName);
							if (f.exists())
							{
								LOGWARNING("Bank already exists with name " << f.getFileName());
								return;
							}

							f.createDirectory();
							updateLibraries(false);
							currentLibrary->setValueWithKey(f.getFileName());
						}
					}
				), true);
			}
			else
			{
				currentBank->setValue(1, false, true); //force reload bank
			}
		}
	}
	else if (c == currentBank)
	{
		updateBank();
	}
	else if (c == nextBank)
	{
		currentBank->setValue(currentBank->intValue() + 1);
	}
	else if (c == prevBank)
	{
		currentBank->setValue(currentBank->intValue() - 1);
	}
	else if (c == bankDescription)
	{
		if (!bankFolder.exists() && bankFolder != File()) bankFolder.createDirectory();
		File newFolder = bankFolder.getParentDirectory().getChildFile(currentBank->stringValue() + "-" + bankDescription->stringValue());
		bankFolder.moveFileTo(newFolder);
		bankFolder = newFolder;
	}
	else if (c == playMode)
	{
		resetAllNotes();
	}
	else if (c == clearLastRecordedTrigger)
	{
		if (clearLastMode->getValueDataAsEnum<ClearMode>() == LAST_RECORDED)
		{
			clearNote(lastRecordedNote);
			lastRecordedNote = -1;
		}
		else
		{
			clearNote(lastPlayedNote);
			lastPlayedNote = -1;
		}
	}
	else if (c == clearAllNotesTrigger)
	{
		clearAllNotes();
	}
	else if (c == loadBankTrigger)
	{
		loadBankSamples();
	}
	else if (c == saveBankTrigger)
	{
		saveBankSamples();
	}
	else if (c == showFolderTrigger)
	{
		bankFolder.startAsProcess();
	}
	else if (c == computeAutoKeysTrigger)
	{
		computeAutoKeys();
	}
}

void SamplerNode::controllableStateChanged(Controllable* c)
{
	Node::controllableStateChanged(c);
}

void SamplerNode::midiMessageReceived(MIDIInterface* i, const MidiMessage& m)
{
	//Node::midiMessageReceived(i, m);
	midiCollector.addMessageToQueue(m); //ugly hack to have at least events sorted, but sampleNumber should be exact
}


void SamplerNode::handleNoteOn(MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity)
{
	PlayMode pm = playMode->getValueDataAsEnum<PlayMode>();
	HitMode hm = hitMode->getValueDataAsEnum<HitMode>();

	SamplerNote* sn = samplerNotes[midiNoteNumber];

	if (clearMode->boolValue()) //clearing
	{
		clearNote(midiNoteNumber);
	}
	else if (!samplerNotes[midiNoteNumber]->hasContent()) //record and proxy autokey playing
	{
		int closestNote = -1;
		if (autoKeyLiveMode->boolValue())
		{
			for (int i = 1; i <= 127; i++)
			{
				int lowI = midiNoteNumber - i;
				int highI = midiNoteNumber + i;
				if (lowI >= 0 && samplerNotes[lowI]->hasContent())
				{
					closestNote = lowI;
					break;
				}
				else if (highI < 128 && samplerNotes[highI]->hasContent())
				{
					closestNote = highI;
					break;
				}
			}
		}

		if (closestNote != -1)
		{
			NoteState ns = sn->state->getValueDataAsEnum<NoteState>();
			if (hm == TOGGLE && ns == PLAYING)
			{
				samplerNotes[midiNoteNumber]->adsr.gate(0);
				samplerNotes[midiNoteNumber]->state->setValueWithData(FILLED);
			}
			else if (ns == PLAYING && (hm == HIT_RESET || hm == HIT_FULL))
			{
				if (hm == HIT_RESET)
				{
					sn->jumpGhostSample = -1;
					sn->playingSample = 0;
					sn->rtPitchReadSample = 0;
				}
			}
			else
			{
				double shift = MidiMessage::getMidiNoteInHertz(midiNoteNumber) / MidiMessage::getMidiNoteInHertz(closestNote);
				samplerNotes[midiNoteNumber]->setAutoKey(samplerNotes[closestNote], shift);

				sn->velocity = velocity;
				sn->adsr.gate(1);
				lastPlayedNote = midiNoteNumber;
				sn->state->setValueWithData(PLAYING);
			}
		}
		else
		{
			startRecording(midiNoteNumber);
		}
	}
	else //filled note playing
	{
		NoteState ns = sn->state->getValueDataAsEnum<NoteState>();
		if (hm == TOGGLE && ns == PLAYING)
		{
			samplerNotes[midiNoteNumber]->adsr.gate(0);
			samplerNotes[midiNoteNumber]->state->setValueWithData(FILLED);
			//handleNoteOff(source, midiChannel, midiNoteNumber, velocity);
		}
		else if (ns == PLAYING && (hm == HIT_RESET || hm == HIT_FULL))
		{
			if (hm == HIT_RESET)
			{
				sn->jumpGhostSample = -1;
				sn->playingSample = 0;
			}
		}
		else
		{
			if (pm == HIT_LOOP || pm == HIT_ONESHOT)
			{
				if (sn->adsr.getState() != CurvedADSR::env_idle) sn->jumpGhostSample = sn->playingSample;
				sn->playingSample = 0;
				sn->oneShotted = false;
			}

			if (sn->isProxyNote())
			{
				sn->pitcher->reset(); //here even with peek
				sn->rtPitchReadSample = sn->playingSample;
			}

			sn->velocity = velocity;
			sn->adsr.gate(1);
			lastPlayedNote = midiNoteNumber;
			sn->state->setValueWithData(PLAYING);
		}
	}
}

void SamplerNode::handleNoteOff(MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity)
{
	HitMode hm = hitMode->getValueDataAsEnum<HitMode>();

	if (samplerNotes[midiNoteNumber]->state->getValueDataAsEnum<NoteState>() == RECORDING) stopRecording();
	else if (samplerNotes[midiNoteNumber]->hasContent() || samplerNotes[midiNoteNumber]->isProxyNote())
	{
		if (hm != PIANO) return;
		samplerNotes[midiNoteNumber]->adsr.gate(0);
		samplerNotes[midiNoteNumber]->state->setValueWithData(FILLED);
	}
}


int SamplerNode::getFadeNumSamples(int fadeMS)
{
	return fadeMS * AudioManager::getInstance()->currentSampleRate / 1000;
}

void SamplerNode::updateLibraries(bool loadAfter)
{
	File folder = libraryFolder->getFile();
	if (!folder.exists() || !folder.isDirectory()) return;

	var oldData = currentLibrary->getValueData();

	isUpdatingLibrary = true;
	currentLibrary->clearOptions();
	Array<File> files = folder.findChildFiles(File::findDirectories, false);
	for (auto& f : files)
	{
		currentLibrary->addOption(f.getFileName(), f.getFullPathName());
	}

	currentLibrary->addOption("Add new...", "new");

	if (oldData.isVoid()) currentLibrary->setValue(currentLibrary->enumValues[0]->key);
	else if (oldData.toString() != "new") currentLibrary->setValue(oldData);

	isUpdatingLibrary = false;

	if (loadAfter) currentBank->setValue(1, false, true);
}

void SamplerNode::updateBank()
{
	File libRootFolder = libraryFolder->getFile();
	if (!libRootFolder.exists() || !libRootFolder.isDirectory()) return;

	String libName = currentLibrary->getValueData().toString();
	if (libName.isEmpty())
	{
		NLOG(niceName, "No library selected in folder, please choose or create a library to load and save.");
		return;
	}

	File libFolder = libRootFolder.getChildFile(libName);
	File bFolder;
	Array<File> bFolders = libFolder.findChildFiles(File::findDirectories, false);

	String desc = "";
	for (auto& bf : bFolders)
	{
		StringArray split;
		split.addTokens(bf.getFileName(), "-", "");
		if (split[0] == String(currentBank->intValue()))
		{
			desc = split.size() > 0 ? split[1] : "";
			bFolder = bf;
			break;
		}
	}

	if (bFolder == File())
	{
		bFolder = libFolder.getChildFile(String(currentBank->intValue()));
		bFolder.createDirectory();
	}

	bankFolder = bFolder;
	bankDescription->setValue(desc);

	if (autoLoadBank->boolValue()) loadBankSamples();
}

void SamplerNode::saveBankSamples()
{
	if (!bankFolder.exists()) bankFolder.createDirectory();

	Array<File> filesToDelete = bankFolder.findChildFiles(File::TypesOfFileToFind::findFiles, false);
	for (auto& f : filesToDelete) f.deleteFile();
	bankFolder.createDirectory();

	LOG("Exporting samples...");

	if (!bankFolder.exists()) return;

	int numSamplesExported = 0;
	for (int i = 0; i < samplerNotes.size(); i++)
	{
		SamplerNote* n = samplerNotes[i];
		if (!n->hasContent() || n->isProxyNote()) continue;

		File f = bankFolder.getChildFile(String(i) + ".wav");
		if (f.exists()) f.deleteFile();

		if (auto fileStream = std::unique_ptr<FileOutputStream>(f.createOutputStream()))
		{
			// Now create a WAV writer object that writes to our output stream...
			WavAudioFormat wavFormat;
			if (std::unique_ptr<AudioFormatWriter> writer = std::unique_ptr<AudioFormatWriter>(wavFormat.createWriterFor(fileStream.get(), processor->getSampleRate(), n->buffer.getNumChannels(), 16, {}, 0)))
			{
				fileStream.release(); // (passes responsibility for deleting the stream to the writer object that is now using it)
				bool result = writer->writeFromAudioSampleBuffer(n->buffer, 0, n->buffer.getNumSamples());
				if (!result) LOGWARNING("Error writing note " << i);
				else numSamplesExported++;
				writer->flush();
			}
			else
			{
				LOGWARNING("Could not create a writer for " << f.getFileName());
			}

		}
		else
		{
			LOGWARNING("Could not open " << f.getFileName() << " for writing");
		}
	}

	NLOG(niceName, numSamplesExported << " sampler notes exported to " << bankFolder.getFullPathName());
}

void SamplerNode::loadBankSamples()
{
	if (!bankFolder.exists() || !bankFolder.isDirectory())
	{
		NLOGWARNING(niceName, "Samples folder is not valid");
		return;
	}

	LOG("Importing samples...");

	int numSamplesImported = 0;
	for (int i = 0; i < samplerNotes.size(); i++)
	{
		SamplerNote* n = samplerNotes[i];
		if (n->hasContent()) clearNote(i);

		File f = bankFolder.getChildFile(String(i) + ".wav");

		if (!f.existsAsFile()) continue;

		if (auto fileStream = std::unique_ptr<FileInputStream>(f.createInputStream()))
		{
			WavAudioFormat wavFormat;
			if (std::unique_ptr<AudioFormatReader> reader = std::unique_ptr<AudioFormatReader>(wavFormat.createReaderFor(fileStream.get(), false)))
			{
				fileStream.release(); // (passes responsibility for deleting the stream to the writer object that is now using it)
				n->buffer.setSize(reader->numChannels, reader->lengthInSamples);
				reader->read(n->buffer.getArrayOfWritePointers(), reader->numChannels, 0, n->buffer.getNumSamples());
				n->state->setValueWithData(FILLED);
				numSamplesImported++;
			}
		}
	}

	NLOG(niceName, numSamplesImported << " sampler notes imported from " << bankFolder.getFullPathName());
}


var SamplerNode::getJSONData()
{
	var data = Node::getJSONData();
	data.getDynamicObject()->setProperty("viewStartKey", viewStartKey);
	return data;
}

void SamplerNode::loadJSONDataItemInternal(var data)
{
	Node::loadJSONDataItemInternal(data);
	viewStartKey = data.getProperty("viewStartKey", viewStartKey);
}

void SamplerNode::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock)
{
	if (sampleRate != 0) midiCollector.reset(sampleRate);

	for (int i = 0; i < 128; i++)
	{
		samplerNotes[i]->adsr.setAttackRate(attack->floatValue() * sampleRate);
		samplerNotes[i]->adsr.setTargetRatioA(attackCurve->floatValue());
		samplerNotes[i]->adsr.setDecayRate(decay->floatValue() * sampleRate);
		samplerNotes[i]->adsr.setSustainLevel(sustain->gain);
		samplerNotes[i]->adsr.setReleaseRate(release->floatValue() * sampleRate);
		samplerNotes[i]->adsr.setTargetRatioDR(releaseCurve->floatValue());
	}
	//for (int i = 0; i < 128; i++) samplerNotes[i]->adsr.setSampleRate(sampleRate);
}

void SamplerNode::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
	if (fadeTimeMS->intValue() > 0) ringBuffer->writeSamples(buffer, 0, jmin(buffer.getNumSamples(), ringBuffer->bufferSize));

	int blockSize = buffer.getNumSamples();

	if (isRecording->boolValue())
	{
		GenericScopedTryLock<SpinLock> lock(recLock);

		if (lock.isLocked())
		{
			SamplerNote* samplerNote = samplerNotes[recordingNote];

			for (int i = 0; i < buffer.getNumChannels(); i++)
			{
				samplerNote->buffer.copyFrom(i, recordedSamples, buffer, i, 0, blockSize);
			}

			recordedSamples += blockSize;
		}
	}

	if (!monitor->boolValue()) buffer.clear();

	keyboardState.processNextMidiBuffer(inMidiBuffer, 0, buffer.getNumSamples(), false);

	AudioSampleBuffer tmpNoteBuffer(buffer.getNumChannels(), blockSize);

	PlayMode pm = playMode->getValueDataAsEnum<PlayMode>();

	for (int i = 0; i < 128; i++)
	{
		SamplerNote* s = samplerNotes[i];

		NoteState st = s->state->getValueDataAsEnum<NoteState>();
		if (st == EMPTY || st == RECORDING || st == PROCESSING) continue;



		if (s->adsr.getState() == CurvedADSR::env_idle)
		{
			if (pm == PEEK)
			{
				s->playingSample += blockSize;
				if (s->playingSample >= s->buffer.getNumSamples()) s->playingSample = 0;
			}
			else if (s->isProxyNote())
			{
				s->setAutoKey(nullptr);
				s->state->setValueWithData(EMPTY);
			}

			continue;
		}


		if (pm != HIT_ONESHOT || !s->oneShotted)
		{
			AudioSampleBuffer* targetBuffer = &s->buffer;
			int targetReadSample = s->playingSample;

			if (s->isProxyNote())
			{

				AudioBuffer<float> tmpBuffer(s->autoKeyFromNote->buffer.getNumChannels(), blockSize);
				auto readPointers = tmpBuffer.getArrayOfReadPointers();

				DBG("Rt pitch sample before");
				GenericScopedLock pLock(s->pitcherLock);
				while (s->pitcher->available() < blockSize)
				{

					for (int ch = 0; ch < buffer.getNumChannels(); ch++) tmpBuffer.copyFrom(ch, 0, s->autoKeyFromNote->buffer.getReadPointer(ch, s->rtPitchReadSample), blockSize);
					s->pitcher->process(readPointers, tmpBuffer.getNumSamples(), false);
					s->rtPitchReadSample += blockSize;

					if (s->rtPitchReadSample >= s->autoKeyFromNote->buffer.getNumSamples())
					{
						DBG("Reset RT read sample");
						s->rtPitchReadSample = 0;

						if (pm == HIT_ONESHOT)
						{
							DBG("One shot break");
							s->oneShotted = true;
							s->jumpGhostSample = -1;
							s->playingSample = 0;
							s->oneShotted = false;
							s->state->setValueWithData(s->hasContent() ? NoteState::FILLED : NoteState::EMPTY);
							break; //stop here
						}
					}
				}

				DBG("Available : " << s->pitcher->available() << " / " << blockSize);
				s->pitcher->retrieve(s->rtPitchedBuffer.getArrayOfWritePointers(), s->rtPitchedBuffer.getNumSamples());

				targetBuffer = &s->rtPitchedBuffer;
				targetReadSample = 0; //read from rtbuffer
			}


			//String dbg;
			//dbg << "[" << i << "] Jump ghost sample : " << s->jumpGhostSample << ", playingSample : " << s->playingSample << ", adsr : " << s->adsr.getOutput() << ", velocity " << s->velocity;

			if (s->jumpGhostSample != -1 && s->jumpGhostSample != s->playingSample && s->jumpGhostSample < s->buffer.getNumSamples())
			{
				//dbg += " > Interpolate";
				for (int j = 0; j < buffer.getNumChannels(); j++)
				{
					tmpNoteBuffer.copyFromWithRamp(j, 0, targetBuffer->getReadPointer(j, s->jumpGhostSample), blockSize, 1, 0);
					tmpNoteBuffer.addFromWithRamp(j, 0, targetBuffer->getReadPointer(j, s->playingSample), blockSize, 0, 1);
				}

				s->jumpGhostSample = -1;
			}
			else
			{
				for (int j = 0; j < buffer.getNumChannels(); j++)
				{
					if (pm == HIT_ONESHOT)
					{
						if (s->playingSample == targetBuffer->getNumSamples() - blockSize)
						{
							tmpNoteBuffer.copyFromWithRamp(j, 0, targetBuffer->getReadPointer(j, targetReadSample), blockSize, 1, 0);
						}
						else if (s->playingSample == 0)
						{
							tmpNoteBuffer.copyFromWithRamp(j, 0, targetBuffer->getReadPointer(j, targetReadSample), blockSize, 0, 1);
						}
						else
						{
							tmpNoteBuffer.copyFrom(j, 0, *targetBuffer, j, targetReadSample, blockSize);
						}
					}
					else
					{
						tmpNoteBuffer.copyFrom(j, 0, *targetBuffer, j, targetReadSample, blockSize);
					}
				}
			}

			s->adsr.applyEnvelopeToBuffer(tmpNoteBuffer, 0, blockSize);
			for (int j = 0; j < buffer.getNumChannels(); j++)
			{
				buffer.addFromWithRamp(j, 0, tmpNoteBuffer.getReadPointer(j), blockSize, s->prevVelocity, s->velocity);
			}

			s->prevVelocity = s->velocity;

			s->playingSample += blockSize;

			int numSamplesToCheck = s->isProxyNote() ? s->autoKeyFromNote->buffer.getNumSamples() : targetBuffer->getNumSamples();
			if (!s->isProxyNote() && s->playingSample >= numSamplesToCheck)
			{
				if (pm == HIT_ONESHOT)
				{
					//s->oneShotted = true;
					//s->jumpGhostSample = -1;

					//HitMode hm = hitMode->getValueDataAsEnum<HitMode>();
					//if (hm == HIT_FULL || hm == HIT_RESET || hm == TOGGLE)
					//{
					s->reset();
					//}
				}
				s->playingSample = 0;
			}
		}
	}

	if (!inMidiBuffer.isEmpty())
	{
		for (auto& c : outMidiConnections) c->destNode->receiveMIDIFromInput(this, inMidiBuffer);
	}

	inMidiBuffer.clear();
}


BaseNodeViewUI* SamplerNode::createViewUI()
{
	return new SamplerNodeViewUI(this);
}

SamplerNode::SamplerNote::SamplerNote() :
	Thread("Sampler Stretcher")
{
}

SamplerNode::SamplerNote::~SamplerNote()
{
	stopThread(1000);
}

void SamplerNode::SamplerNote::setAutoKey(SamplerNote* remoteNote, double shift)
{
	autoKeyFromNote = remoteNote;
	if (autoKeyFromNote == nullptr)
	{
		//pitcher.reset();
		rtPitchedBuffer.clear();
		return;
	}

	GenericScopedLock lock(pitcherLock);

	if (pitcher == nullptr)
	{
		pitcher.reset(new RubberBand::RubberBandStretcher(Transport::getInstance()->sampleRate, autoKeyFromNote->buffer.getNumChannels(),
			RubberBand::RubberBandStretcher::OptionProcessRealTime
			| RubberBand::RubberBandStretcher::OptionStretchPrecise
			| RubberBand::RubberBandStretcher::OptionFormantPreserved
			| RubberBand::RubberBandStretcher::OptionWindowLong
			| RubberBand::RubberBandStretcher::OptionTransientsCrisp
			| RubberBand::RubberBandStretcher::OptionPitchHighQuality
			| RubberBand::RubberBandStretcher::OptionChannelsTogether
		));
	}

	pitcher->setMaxProcessSize(Transport::getInstance()->blockSize);
	pitcher->setPitchScale(shift);
	//LOG("Set with pitchScale : " << shift);
	rtPitchReadSample = 0;
	rtPitchedBuffer.setSize(autoKeyFromNote->buffer.getNumChannels(), Transport::getInstance()->blockSize);
}

void SamplerNode::SamplerNote::computeAutoKey(SamplerNote* remoteNote, double shift, int fadeSamples)
{
	shifting = shift;
	fadeNumSamples = fadeSamples;
	autoKeyFromNote = remoteNote;

	startThread();

}

void SamplerNode::SamplerNote::run()
{
	state->setValueWithData(NoteState::PROCESSING);

	// Set up a temporary buffer to hold the resampled audio
	const int numChannels = autoKeyFromNote->buffer.getNumChannels();
	const int numSamples = autoKeyFromNote->buffer.getNumSamples();
	const float* const* channelData = autoKeyFromNote->buffer.getArrayOfReadPointers();
	buffer = AudioSampleBuffer(numChannels, 0);

	// Create a Rubber Band instance with the desired shift
	RubberBand::RubberBandStretcher rubberBand(Transport::getInstance()->sampleRate, autoKeyFromNote->buffer.getNumChannels(),
		RubberBand::RubberBandStretcher::OptionProcessOffline
		| RubberBand::RubberBandStretcher::OptionStretchPrecise
		| RubberBand::RubberBandStretcher::OptionFormantPreserved
		| RubberBand::RubberBandStretcher::OptionWindowLong
		| RubberBand::RubberBandStretcher::OptionTransientsCrisp
		| RubberBand::RubberBandStretcher::OptionPitchHighQuality
		| RubberBand::RubberBandStretcher::OptionChannelsTogether
	);

	double timeRatio = (numSamples + fadeNumSamples) * 1.0 / numSamples;

	rubberBand.setPitchScale(shifting);
	rubberBand.setTimeRatio(timeRatio);
	rubberBand.setExpectedInputDuration(autoKeyFromNote->buffer.getNumSamples());

	rubberBand.study(channelData, numSamples, true);
	rubberBand.process(channelData, numSamples, true);
	while (rubberBand.available() > 0)
	{
		juce::AudioBuffer<float> tmpBuffer(numChannels, rubberBand.available());
		int numRetrieved = rubberBand.retrieve(tmpBuffer.getArrayOfWritePointers(), rubberBand.available());

		int curSize = buffer.getNumSamples();
		buffer.setSize(numChannels, curSize + numRetrieved, true, false, true);
		for (int i = 0; i < numChannels; i++) buffer.copyFrom(i, curSize, tmpBuffer, i, 0, numRetrieved);
	}

	jassert(buffer.getNumSamples() == numSamples + fadeNumSamples);


	if (fadeNumSamples > 0)
	{
		for (int i = 0; i < buffer.getNumChannels(); i++)
		{
			buffer.applyGainRamp(0, fadeNumSamples, 0, 1);
			buffer.addFromWithRamp(i, 0, buffer.getReadPointer(i, numSamples), fadeNumSamples, 1, 0);
		}
	}

	buffer.setSize(numChannels, numSamples, true, true, true);

	autoKeyFromNote = nullptr;
	state->setValueWithData(NoteState::FILLED);
}

void SamplerNode::SamplerNote::reset()
{
	//adsr.reset();
	adsr.gate(0);
	setAutoKey(nullptr);
	jumpGhostSample = -1;
	playingSample = 0;
	oneShotted = false;
	state->setValueWithData(hasContent() ? NoteState::FILLED : NoteState::EMPTY);
}