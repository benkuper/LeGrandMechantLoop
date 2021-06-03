/*
  ==============================================================================

	AnalysisNode.cpp
	Created: 3 Jun 2021 3:36:13pm
	Author:  bkupe

  ==============================================================================
*/

#include "AnalysisNode.h"

AnalysisNode::AnalysisNode(var params) :
	Node(getTypeString(), params, true, false, false, false),
	noteCC("Note")
{
	saveAndLoadRecursiveData = true;

	activityThreshold = addFloatParameter("Activity Threshold", "Threshold to consider activity from the source.\nAnalysis will compute only if volume is greater than this parameter", .1f, 0, 1);
	pitchDetectionMethod = addEnumParameter("Pitch Detection Method", "Choose how to detect the pitch.\nNone will disable the detection (for performance),\nMPM is better suited for monophonic sounds,\nYIN is better suited for high-pitched voices and music");
	pitchDetectionMethod->addOption("None", NONE)->addOption("MPM", MPM)->addOption("YIN", YIN);
	keepLastDetectedValues = addBoolParameter("Keep Values", "Keep last detected values when no activity detected.", false);


	//Pitch Detection
	frequency = noteCC.addFloatParameter("Freq", "Freq", 0, 0, 2000);
	pitch = noteCC.addIntParameter("Pitch", "Pitch", 0, 0, 300);

	note = noteCC.addEnumParameter("Note", "Detected note");
	note->addOption("-", -1);
	for (int i = 0; i < 12; ++i) note->addOption(MIDIManager::getNoteName(i, false), i);

	octave = noteCC.addIntParameter("Octave", "Detected octave", 0, 0, 10);

	//FFT
	addChildControllableContainer(&noteCC);
	addChildControllableContainer(&analyzerManager);

	setAudioInputs(1);
}

AnalysisNode::~AnalysisNode()
{
}

void AnalysisNode::onContainerParameterChangedInternal(Parameter* p)
{
	if (p == pitchDetectionMethod)
	{
		PitchDetectionMethod pdm = pitchDetectionMethod->getValueDataAsEnum<PitchDetectionMethod>();

		switch (pdm)
		{
		case NONE: pitchDetector.reset(nullptr); break;
		case MPM: pitchDetector.reset(new PitchMPM(processor->getSampleRate(), processor->getBlockSize())); break;
		case YIN: pitchDetector.reset(new PitchYIN(processor->getSampleRate(), processor->getBlockSize())); break;
		}

	}
}

void AnalysisNode::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock)
{
}

void AnalysisNode::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
	float rms = buffer.getRMSLevel(0, 0, buffer.getNumSamples());
	if (rms > activityThreshold->floatValue())
	{
		if (pitchDetector != nullptr)
		{
			if ((int)pitchDetector->getBufferSize() != buffer.getNumSamples()) pitchDetector->setBufferSize(buffer.getNumSamples());

			float freq = pitchDetector->getPitch(buffer.getReadPointer(0));
			frequency->setValue(freq);
			int pitchNote = getNoteForFrequency(freq);
			pitch->setValue(pitchNote);

			note->setValueWithKey(MIDIManager::getNoteName(pitchNote, false));
			octave->setValue(floor(pitchNote / 12.0));
		}
	}
	else
	{
		if (!keepLastDetectedValues->boolValue())
		{
			frequency->setValue(0);
			pitch->setValue(0);
			note->setValueWithKey("-");
		}
	}

	//Analysis
	analyzerManager.process(buffer.getReadPointer(0), buffer.getNumSamples());
}

int AnalysisNode::getNoteForFrequency(float freq)
{
	return (int)(69 + 12 * log2(freq / 440)); //A = 440
}

