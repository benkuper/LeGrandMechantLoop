/*
  ==============================================================================

	SamplerNode.cpp
	Created: 14 Apr 2021 8:40:43am
	Author:  bkupe

  ==============================================================================
*/

#include "SamplerNode.h"
#include "ui/SamplerNodeUI.h"
#include "../../Connection/NodeConnection.h"
#include "Engine/AudioManager.h"

SamplerNode::SamplerNode(var params) :
	Node(getTypeStringStatic(), params, true, true, false, true),
	currentDevice(nullptr),
	recordingNote(-1),
	lastRecordedNote(-1),
	lastPlayedNote(-1),
	recordedSamples(0),
	noteStatesCC("Notes"),
	viewStartKey(20)
{
	numChannels = addIntParameter("Num Channels", "Num Channels to use for recording and playing", 1);

	monitor = addBoolParameter("Monitor", "Monitor input audio", true);

	playMode = addEnumParameter("Play Mode", "How samples are treated");
	playMode->addOption("Hit", HIT)->addOption("Loop", LOOP)->addOption("Hold", HOLD);
	midiParam = new MIDIDeviceParameter("MIDI Device", true, false);
	ControllableContainer::addParameter(midiParam);

	fadeTimeMS = addIntParameter("Fade Time MS", "Time to fade between start and end of recording", 50);

	isRecording = addBoolParameter("Is Recording", "Is recording a note ?", false);
	isRecording->setControllableFeedbackOnly(true);

	clearMode = addBoolParameter("Clear Mode", "When checked, recorded or played notes are cleared depending on clearing mode", false);

	clearLastMode = addEnumParameter("Clearing Lat Mode", "The way to clear when clear last is triggered");
	clearLastMode->addOption("Last Played", LAST_PLAYED)->addOption("Last Recorded", LAST_RECORDED);

	clearLastRecordedTrigger = addTrigger("Clear Last", "Clear Last recorded or played note");
	clearAllNotesTrigger = addTrigger("Clear All Notes", "Clear all recorded notes");

	attack = addFloatParameter("Attack", "Time of the attack in seconds", .01f, 0);
	attack->defaultUI = FloatParameter::TIME;

	attackCurve = addFloatParameter("Attack Curve", "Bend the attack", 0, 0, 0.1);

	decay = addFloatParameter("Decay", "Time of decay in seconds", .2f, 0);
	decay->defaultUI = FloatParameter::TIME;

	sustain = new DecibelFloatParameter("Sustain", "Sustain dB");
	addParameter(sustain);

	release = addFloatParameter("Release", "Time of the release in seconds", .2f, 0);
	release->defaultUI = FloatParameter::TIME;

	releaseCurve = addFloatParameter("ReleaseCurve", "Bend the release", 0, 0, 0.1);

	keyboardState.addListener(this);

	int sr = processor->getSampleRate();

	for (int i = 0; i < 128; i++)
	{
		samplerNotes.add(new SamplerNote());
		samplerNotes[i]->buffer.clear();
		samplerNotes[i]->adsr.setAttackRate(attack->floatValue() * sr);
		samplerNotes[i]->adsr.setTargetRatioA(attackCurve->floatValue());
		samplerNotes[i]->adsr.setDecayRate(decay->floatValue() * sr);
		samplerNotes[i]->adsr.setSustainLevel(sustain->gain);
		samplerNotes[i]->adsr.setReleaseRate(release->floatValue() * sr);
		samplerNotes[i]->adsr.setTargetRatioDR(releaseCurve->floatValue());

		samplerNotes[i]->state = noteStatesCC.addEnumParameter(MidiMessage::getMidiNoteName(i,true, true, 3), "State for this note");
		samplerNotes[i]->state->addOption("Empty", EMPTY)->addOption("Recording", RECORDING)->addOption("Filled", FILLED)->addOption("Playing", PLAYING);
		samplerNotes[i]->state->setControllableFeedbackOnly(true);
	}


	//for (float i = -50; i <= -6; i+=.25f)
	//{
	//	float values = DecibelsHelpers::decibelsToValue(i);
	//	float decibels = DecibelsHelpers::valueToDecibels(values);
	//	LOG(i << "\t" << values << "\t" << decibels);
	//}

	updateBuffers();

	setAudioInputs(numChannels->intValue());
	setAudioOutputs(numChannels->intValue());

	addChildControllableContainer(&noteStatesCC);

	setMIDIIO(true, true);
}

SamplerNode::~SamplerNode()
{
	setMIDIDevice(nullptr);
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
		n->buffer.setSize(0,0);
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

	ScopedSuspender sp(processor);
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

	ScopedSuspender sp(processor);
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

			for(int i = 0; i < samplerNote->buffer.getNumChannels(); i++)
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

void SamplerNode::exportSampleSet(File folder)
{
}

void SamplerNode::importSampleSet(File folder)
{
}

void SamplerNode::setMIDIDevice(MIDIInputDevice* d)
{
	if (currentDevice == d) return;
	if (currentDevice != nullptr)
	{
		currentDevice->removeMIDIInputListener(this);
	}

	currentDevice = d;

	if (currentDevice != nullptr)
	{
		currentDevice->addMIDIInputListener(this);
	}
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
}

void SamplerNode::onContainerParameterChangedInternal(Parameter* p)
{
	Node::onContainerParameterChangedInternal(p);
	if (p == midiParam) setMIDIDevice(midiParam->inputDevice);
	else if (p == numChannels)
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
			if(p == attack) samplerNotes[i]->adsr.setAttackRate(attack->floatValue() * sr);
			else if(p == attackCurve) samplerNotes[i]->adsr.setTargetRatioA(attackCurve->floatValue());
			else if(p == decay) samplerNotes[i]->adsr.setDecayRate(decay->floatValue() * sr);
			else if (p == sustain) samplerNotes[i]->adsr.setSustainLevel(sustain->gain);
			else if (p == release) samplerNotes[i]->adsr.setReleaseRate(release->floatValue() * sr);
			else if (p == releaseCurve) samplerNotes[i]->adsr.setTargetRatioDR(releaseCurve->floatValue());
		}

	}
	else if (p == fadeTimeMS)
	{
		updateRingBuffer();
	}
}

void SamplerNode::controllableStateChanged(Controllable* c)
{
	Node::controllableStateChanged(c);
}

void SamplerNode::midiMessageReceived(const MidiMessage& m)
{
	midiCollector.addMessageToQueue(m); //ugly hack to have at least events sorted, but sampleNumber should be exact
}


void SamplerNode::handleNoteOn(MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity)
{
	PlayMode pm = playMode->getValueDataAsEnum<PlayMode>();

	if (clearMode->boolValue())
	{
		clearNote(midiNoteNumber);
	}else if (!samplerNotes[midiNoteNumber]->hasContent())
	{
		startRecording(midiNoteNumber);
	}
	else
	{
		if (pm == HIT) samplerNotes[midiNoteNumber]->playingSample = 0;
		samplerNotes[midiNoteNumber]->velocity = velocity;
		samplerNotes[midiNoteNumber]->adsr.gate(1);
		lastPlayedNote = midiNoteNumber;
		samplerNotes[midiNoteNumber]->state->setValueWithData(PLAYING);
	}

}

void SamplerNode::handleNoteOff(MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity)
{
	if (samplerNotes[midiNoteNumber]->state->getValueDataAsEnum<NoteState>() == RECORDING) stopRecording();
	else if(samplerNotes[midiNoteNumber]->hasContent())
	{
		samplerNotes[midiNoteNumber]->adsr.gate(0);
		samplerNotes[midiNoteNumber]->state->setValueWithData(FILLED);
	}
}


int SamplerNode::getFadeNumSamples()
{
	return fadeTimeMS->intValue() * AudioManager::getInstance()->currentSampleRate / 1000;
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
		SamplerNote * samplerNote = samplerNotes[recordingNote];

		for (int i = 0; i < buffer.getNumChannels(); i++)
		{
			samplerNote->buffer.copyFrom(i, recordedSamples, buffer, i, 0, blockSize);
		}

		recordedSamples += blockSize;
	}

	if (!monitor->boolValue()) buffer.clear();

	if (currentDevice != nullptr)
	{
		midiCollector.removeNextBlockOfMessages(inMidiBuffer, buffer.getNumSamples());
	}

	keyboardState.processNextMidiBuffer(inMidiBuffer, 0, buffer.getNumSamples(), false);

	AudioSampleBuffer tmpNoteBuffer(buffer.getNumChannels(), blockSize);

	PlayMode pm = playMode->getValueDataAsEnum<PlayMode>();

	for (int i = 0; i < 128; i++)
	{
		SamplerNote* s = samplerNotes[i];

		NoteState st = s->state->getValueDataAsEnum<NoteState>();
		if(st == EMPTY || st == RECORDING) continue;

		if (s->adsr.getState() == CurvedADSR::env_idle)
		{
			if (pm == LOOP)
			{
				s->playingSample += blockSize;
				if (s->playingSample >= s->buffer.getNumSamples()) s->playingSample = 0;
			}
			continue;
		}

		for (int j = 0; j < buffer.getNumChannels(); j++)
		{
			tmpNoteBuffer.copyFrom(j, 0, s->buffer, j, s->playingSample, blockSize);
		}
			
		s->adsr.applyEnvelopeToBuffer(tmpNoteBuffer, 0, blockSize);
		for (int j = 0; j < buffer.getNumChannels(); j++)
		{
			buffer.addFrom(j, 0, tmpNoteBuffer, j, 0, blockSize, s->velocity);
		}

		s->playingSample += blockSize;
		if (s->playingSample >= s->buffer.getNumSamples()) s->playingSample = 0;

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
