/*
  ==============================================================================

	MixerNode.h
	Created: 15 Nov 2020 8:42:42am
	Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "../../Node.h"



class MixerItem :
	public ControllableContainer
{
public:
	MixerItem(int inputIndex, int outputIndex, bool hasRMS = false);
	~MixerItem();

	int inputIndex;
	int outputIndex;

	float prevGain;
	FloatParameter* gain;
	FloatParameter* rms;
	BoolParameter * active;

	float getGain();
};

class MixerNode :
	public Node
{
public:
	MixerNode(var params = var());
	~MixerNode() {}

	ControllableContainer outItemsCC;
	ControllableContainer itemsCC;

	AudioBuffer<float> tmpBuffer;
	
	void updateAudioInputsInternal() override;
	void updateAudioOutputsInternal() override;

	void updateGainCC(ControllableContainer* cc);

	MixerItem * getMixerItem(int inputIndex, int outputIndex);

	void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;
	
	String getTypeString() const override { return getTypeStringStatic(); }
	static const String getTypeStringStatic() { return "Mixer"; }

	BaseNodeViewUI* createViewUI() override;
};
