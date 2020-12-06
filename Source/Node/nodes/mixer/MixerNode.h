/*
  ==============================================================================

	MixerNode.h
	Created: 15 Nov 2020 8:42:42am
	Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "../../Node.h"

class MixerNode :
	public Node
{
public:
	MixerNode(var params = var());
	~MixerNode() {}

	ControllableContainer rmsCC;
	ControllableContainer outGainsCC;
	ControllableContainer gainRootCC;

	AudioBuffer<float> tmpBuffer;
	
	void updateAudioInputsInternal() override;
	void updateAudioOutputsInternal() override;

	void addGainCC();
	void updateGainCC(ControllableContainer* cc);

	FloatParameter * getGainParameter(int inputIndex, int outputIndex);

	void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;
	
	String getTypeString() const override { return getTypeStringStatic(); }
	static const String getTypeStringStatic() { return "Mixer"; }

	BaseNodeViewUI* createViewUI() override;
};
