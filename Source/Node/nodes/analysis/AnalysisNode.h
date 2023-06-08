/*
  ==============================================================================

	AnalysisNode.h
	Created: 3 Jun 2021 3:36:13pm
	Author:  bkupe

  ==============================================================================
*/

#pragma once

class AnalysisNode :
	public Node
{
public:
	AnalysisNode(var params = var());
	~AnalysisNode();

	FloatParameter* activityThreshold;
	enum PitchDetectionMethod { NONE, MPM, YIN };
	EnumParameter* pitchDetectionMethod;
	BoolParameter* keepLastDetectedValues;

	//Values
	ControllableContainer noteCC;
	FloatParameter* frequency;
	IntParameter* pitch;
	EnumParameter* note;
	IntParameter* octave;

	FFTAnalyzerManager analyzerManager;

	std::unique_ptr<PitchDetector> pitchDetector;

	void onContainerParameterChangedInternal(Parameter* p) override;


	void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
	void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

	DECLARE_TYPE("Audio Analysis");

private:
	int getNoteForFrequency(float freq);
};