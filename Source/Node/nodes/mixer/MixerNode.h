/*
  ==============================================================================

	MixerNode.h
	Created: 15 Nov 2020 8:42:42am
	Author:  bkupe

  ==============================================================================
*/

#pragma once
#include "../../NodeAudioProcessor.h"

class MixerProcessor :
	public GenericNodeAudioProcessor
{
public:
	MixerProcessor(Node * node);
	~MixerProcessor() {}

	IntParameter* numInputs;
	IntParameter* numOutputs;

	ControllableContainer rmsCC;
	ControllableContainer outGainsCC;
	ControllableContainer gainRootCC;

	AudioBuffer<float> tmpBuffer;
	

	void updateInputsFromNodeInternal() override;
	void updateOutputsFromNodeInternal() override;

	void addGainCC();
	void updateGainCC(ControllableContainer* cc);

	FloatParameter * getGainParameter(int inputIndex, int outputIndex);

	void onContainerParameterChanged(Parameter* p) override;

	void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;
	static const String getTypeStringStatic() { return "Mixer"; }

	NodeViewUI* createNodeViewUI() override;
};
