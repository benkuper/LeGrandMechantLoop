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
	isUpdatingBank(false)
{
	includeTriggersInSaveLoad = true;

	numChannels = addIntParameter("Num Channels", "Num Channels to use for recording and playing", 1);

	monitor = addBoolParameter("Monitor", "Monitor input audio", true);

	playMode = addEnumParameter("Play Mode", "How samples are treated");
	playMode->addOption("Hit (Loop)", HIT_LOOP)->addOption("Hit (One shot)", HIT_ONESHOT)->addOption("Peek (Continuous)", PEEK)->addOption("Keep (Hold)", KEEP);

	autoKeyMode = addBoolParameter("Auto Key", "If checked, hitting an unrecorded note will play it repitched from the closest found", false);
	fadeTimeMS = addIntParameter("Fade Time MS", "Time to fade between start and end of recording", 50);

	isRecording = addBoolParameter("Is Recording", "Is recording a note ?", false);
	isRecording->setControllableFeedbackOnly(true);

	clearMode = addBoolParameter("Clear Mode", "When checked, recorded or played notes are cleared depending on clearing mode", false);

	clearLastMode = addEnumParameter("Clear Last Mode", "The way to clear when clear last is triggered");
	clearLastMode->addOption("Last Played", LAST_PLAYED)->addOption("Last Recorded", LAST_RECORDED);

	clearLastRecordedTrigger = addTrigger("Clear Last", "Clear Last recorded or played note");
	clearAllNotesTrigger = addTrigger("Clear All Notes", "Clear all recorded notes");

	attack = addFloatParameter("Attack", "Time of the attack in seconds", .01f, 0);
	attack->defaultUI = FloatParameter::TIME;

	attackCurve = addFloatParameter("Attack Curve", "Bend the attack", 0.06f, 0, 0.1f);

	decay = addFloatParameter("Decay", "Time of decay in seconds", .2f, 0);
	decay->defaultUI = FloatParameter::TIME;

	sustain = new DecibelFloatParameter("Sustain", "Sustain dB");
	addParameter(sustain);

	release = addFloatParameter("Release", "Time of the release in seconds", .2f, 0);
	release->defaultUI = FloatParameter::TIME;

	releaseCurve = addFloatParameter("Release Curve", "Bend the release", 0.001f, 0, 0.1f);

	samplesFolder = addFileParameter("Samples Folder", "To export and import");
	samplesFolder->directoryMode = true;

	currentBank = addEnumParameter("Current Bank", "Select a specific sub folder or the none for the root folder");
	saveSamplesTrigger = addTrigger("Export Samples", "Export all samples at once to the Samples Folder above");
	showFolderTrigger = addTrigger("Show Folder", "Show the folder in explorer");
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
		sn->state->addOption("Empty", EMPTY)->addOption("Recording", RECORDING)->addOption("Filled", FILLED)->addOption("Playing", PLAYING);
		sn->state->setControllableFeedbackOnly(true);

		sn->oneShotted = false;

		samplerNotes.add(sn);
	}


	//for (float i = -50; i <= -6; i+=.25f)
	//{
	//	float values = DecibelsHelpers::decibelsToValue(i);
	//	float decibels = DecibelsHelpers::valueToDecibels(values);
	//	LOG(i << "\t" << values << "\t" << decibels);
	//}

	updateBanks();
	updateBuffers();

	setAudioInputs(numChannels->intValue());
	setAudioOutputs(numChannels->intValue());

	addChildControllableContainer(&noteStatesCC);

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

void SamplerNode::updateBuffers()
{
	ScopedSuspender sp(processor);
	updateRingBuffer();
	for (auto& n : samplerNotes)  n->buffer.setSize(numChannels->intValue(), n->buffer.getNumSamples(), false, true);
}

void SamplerNode::updateRingBuffer()
{
	ScopedSuspender sp(processor);
	ringBuffer.reset(new RingBuffer<float>(numChannels->intValue(), getFadeNumSamples() * 2)); //double to not have overlapping read and write

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

	samplerNote->buffer.setSize(numChannels->intValue(), recNumSamples, false, true);

	int fadeNumSamples = getFadeNumSamples();
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
		int fadeNumSamples = getFadeNumSamples();
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


void SamplerNode::onContainerTriggerTriggered(Trigger* t)
{
	if (t == clearLastRecordedTrigger)
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
	else if (t == clearAllNotesTrigger)
	{
		clearAllNotes();
	}
	else if (t == saveSamplesTrigger)
	{
		exportSamples();
	}
	else if (t == showFolderTrigger)
	{
		samplesFolder->getFile().startAsProcess();
	}
}

void SamplerNode::onContainerParameterChangedInternal(Parameter* p)
{
	Node::onContainerParameterChangedInternal(p);
	if (p == numChannels)
	{
		setAudioInputs(numChannels->intValue());
		setAudioOutputs(numChannels->intValue());
		updateBuffers();
	}
	else if (p == attack || p == decay || p == sustain || p == release || p == attackCurve || p == releaseCurve)
	{
		int sr = processor->getSampleRate();
		for (int i = 0; i < 128; i++)
		{
			if (p == attack) samplerNotes[i]->adsr.setAttackRate(attack->floatValue() * sr);
			else if (p == attackCurve) samplerNotes[i]->adsr.setTargetRatioA(attackCurve->floatValue());
			else if (p == decay) samplerNotes[i]->adsr.setDecayRate(decay->floatValue() * sr);
			else if (p == sustain) samplerNotes[i]->adsr.setSustainLevel(sustain->gain);
			else if (p == release) samplerNotes[i]->adsr.setReleaseRate(release->floatValue() * sr);
			else if (p == releaseCurve) samplerNotes[i]->adsr.setTargetRatioDR(releaseCurve->floatValue());
		}

	}
	else if (p == fadeTimeMS)
	{
		updateRingBuffer();
	}
	else if (p == samplesFolder)
	{
		updateBanks();
		loadSamples();
	}
	else if (p == currentBank)
	{
		if (!isUpdatingBank)
		{
			String b = currentBank->getValueData().toString();
			if (b == "new")
			{
				//create a window with name parameter to create a new folder
				AlertWindow* window = new AlertWindow("Add a Bank", "Add a new bank", AlertWindow::AlertIconType::NoIcon);
				window->addTextEditor("bank", samplesFolder->getFile().getNonexistentChildFile("Bank", "", true).getFileName(), "Bank Name");
				window->addButton("OK", 1, KeyPress(KeyPress::returnKey));
				window->addButton("Cancel", 0, KeyPress(KeyPress::escapeKey));

				window->enterModalState(true, ModalCallbackFunction::create([window, this](int result)
					{

						if (result)
						{
							String bankName = window->getTextEditorContents("bank");
							File f = samplesFolder->getFile().getChildFile(bankName);
							if (f.exists())
							{
								LOGWARNING("Bank already exists with name " << f.getFileName());
								return;
							}

							f.createDirectory();
							updateBanks(false);
							currentBank->setValueWithKey(f.getFileName());
						}
					}
				), true);
			}
			else
			{
				loadSamples();
			}
		}
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

	SamplerNote* sn = samplerNotes[midiNoteNumber];

	if (clearMode->boolValue())
	{
		clearNote(midiNoteNumber);
	}
	else if (!samplerNotes[midiNoteNumber]->hasContent())
	{
		int closestNote = -1;
		if (autoKeyMode->boolValue())
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
			double shift = MidiMessage::getMidiNoteInHertz(midiNoteNumber) / MidiMessage::getMidiNoteInHertz(closestNote);
			samplerNotes[midiNoteNumber]->setAutoKey(samplerNotes[closestNote], shift);

			sn->velocity = velocity;
			sn->adsr.gate(1);
			lastPlayedNote = midiNoteNumber;
			sn->state->setValueWithData(PLAYING);
		}
		else
		{
			startRecording(midiNoteNumber);
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

void SamplerNode::handleNoteOff(MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity)
{
	if (samplerNotes[midiNoteNumber]->state->getValueDataAsEnum<NoteState>() == RECORDING) stopRecording();
	else if (samplerNotes[midiNoteNumber]->hasContent() || samplerNotes[midiNoteNumber]->isProxyNote())
	{
		samplerNotes[midiNoteNumber]->adsr.gate(0);
		samplerNotes[midiNoteNumber]->state->setValueWithData(FILLED);
	}
}


int SamplerNode::getFadeNumSamples()
{
	return fadeTimeMS->intValue() * AudioManager::getInstance()->currentSampleRate / 1000;
}

void SamplerNode::updateBanks(bool loadAfter)
{
	File folder = samplesFolder->getFile();
	if (!folder.exists() || !folder.isDirectory()) return;

	var oldData = currentBank->getValueData();

	isUpdatingBank = true;
	currentBank->clearOptions();
	currentBank->addOption("Default", "");
	Array<File> files = folder.findChildFiles(File::findDirectories, false);
	for (auto& f : files)
	{
		currentBank->addOption(f.getFileName(), f.getFullPathName());
	}

	currentBank->addOption("Add new...", "new");

	if (oldData.isVoid()) oldData = "";
	if (oldData.toString() != "new") currentBank->setValue(oldData);

	isUpdatingBank = false;

	if (loadAfter) loadSamples();
}

void SamplerNode::exportSamples()
{
	File folder = samplesFolder->getFile();

	String bank = currentBank->getValueData().toString();
	if (bank != "new" && bank != "")
	{
		File f(bank);
		if (f.exists()) folder = f;
	}

	if (!folder.exists() && !folder.isDirectory())
	{
		if (Engine::mainEngine->getFile().existsAsFile())
		{
			NLOGWARNING(niceName, "Samples folder is not valid, creating one in the session's directory");
			samplesFolder->setValue(Engine::mainEngine->getFile().getParentDirectory().getChildFile(niceName).getFullPathName());
		}
		else
		{
			NLOGERROR(niceName, "Samples folder is not valid and current session is not saved in a file.");
		}
	}

	Array<File> filesToDelete = folder.findChildFiles(File::TypesOfFileToFind::findFiles, false);
	for (auto& f : filesToDelete) f.deleteFile();
	folder.createDirectory();

	LOG("Exporting samples...");

	if (!folder.exists()) return;

	int numSamplesExported = 0;
	for (int i = 0; i < samplerNotes.size(); i++)
	{
		SamplerNote* n = samplerNotes[i];
		if (!n->hasContent() || n->isProxyNote()) continue;

		File f = folder.getChildFile(String(i) + ".wav");
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

	NLOG(niceName, numSamplesExported << " sampler notes exported to " << folder.getFullPathName());
}

void SamplerNode::loadSamples()
{
	File folder = samplesFolder->getFile();

	String bank = currentBank->getValueData().toString();
	if (bank != "new" && bank != "")
	{
		File f(bank);
		if (f.exists()) folder = f;
	}

	if (!folder.exists() || !folder.isDirectory())
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

		File f = folder.getChildFile(String(i) + ".wav");

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

	NLOG(niceName, numSamplesImported << " sampler notes imported from " << folder.getFullPathName());
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
		if (st == EMPTY || st == RECORDING) continue;



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
				while (s->pitcher->available() < blockSize)
				{

					for (int ch = 0; ch < buffer.getNumChannels(); ch++) tmpBuffer.copyFrom(ch, 0, s->autoKeyFromNote->buffer.getReadPointer(ch, s->rtPitchReadSample), blockSize);

					s->pitcher->process(readPointers, tmpBuffer.getNumSamples(), false);
					s->rtPitchReadSample += blockSize;

					if (s->rtPitchReadSample >= s->autoKeyFromNote->buffer.getNumSamples())
					{
						if (pm == HIT_ONESHOT)
						{
							DBG("One shot break");
							s->oneShotted = true;
							break; //stop here
						}

						DBG("Reset RT read sample");
						s->rtPitchReadSample = 0;
					}
				}
				DBG("Available : " << s->pitcher->available() << " / " << blockSize);
				s->pitcher->retrieve(s->rtPitchedBuffer.getArrayOfWritePointers(), s->rtPitchedBuffer.getNumSamples());

				targetBuffer = &s->rtPitchedBuffer;
				targetReadSample = 0; //read from rtbuffer
			}



			if (s->jumpGhostSample != -1 && s->jumpGhostSample != s->playingSample && s->jumpGhostSample < s->buffer.getNumSamples())
			{
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
				buffer.addFrom(j, 0, tmpNoteBuffer, j, 0, blockSize, s->velocity);
			}

			s->playingSample += blockSize;
			if (s->playingSample >= targetBuffer->getNumSamples())
			{
				if (pm == HIT_ONESHOT)
				{
					s->oneShotted = true;
					s->jumpGhostSample = -1;
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

void SamplerNode::SamplerNote::setAutoKey(SamplerNote* remoteNote, double shift)
{
	autoKeyFromNote = remoteNote;
	if (autoKeyFromNote == nullptr)
	{
		//pitcher.reset();
		rtPitchedBuffer.clear();
		return;
	}

	if (pitcher == nullptr) pitcher.reset(new RubberBand::RubberBandStretcher(Transport::getInstance()->sampleRate, autoKeyFromNote->buffer.getNumChannels(),
		RubberBand::RubberBandStretcher::OptionProcessRealTime
		| RubberBand::RubberBandStretcher::OptionStretchPrecise
		| RubberBand::RubberBandStretcher::OptionFormantPreserved
		| RubberBand::RubberBandStretcher::OptionWindowLong
		| RubberBand::RubberBandStretcher::OptionTransientsCrisp
		| RubberBand::RubberBandStretcher::OptionPitchHighQuality
		| RubberBand::RubberBandStretcher::OptionChannelsTogether
	));
	else pitcher->reset();

	pitcher->setMaxProcessSize(Transport::getInstance()->blockSize);
	pitcher->setPitchScale(shift);
	//LOG("Set with pitchScale : " << shift);
	rtPitchedBuffer.setSize(autoKeyFromNote->buffer.getNumChannels(), Transport::getInstance()->blockSize);
}
