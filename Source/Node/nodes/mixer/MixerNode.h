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
	public VolumeControl
{
public:
	MixerItem(int inputIndex, int outputIndex, bool hasRMS = false);
	~MixerItem();

	int inputIndex;
	int outputIndex;
};

class OutputLineCC :
	public ControllableContainer
{
public:
	OutputLineCC(int index);
	~OutputLineCC();

	int index;

	Array<MixerItem*> mixerItems;
	BoolParameter* exclusiveMode;
	VolumeControl out;

	void setInputNumber(int inputNumber);
	void updateActives(MixerItem * activeItem = nullptr);
	void onContainerParameterChanged(Parameter* p)override;
	void onControllableFeedbackUpdate(ControllableContainer * cc, Controllable * c)override;
};

class MixerNode :
	public Node
{
public:
	MixerNode(var params = var());
	~MixerNode() {}

	ControllableContainer itemsCC;
	Array<OutputLineCC*> outputLines;

	BoolParameter* showOutputGains;
	BoolParameter* showOutputRMS;
	BoolParameter* showOutputActives;
	BoolParameter* showItemGains;
	BoolParameter* showItemActives;

	AudioBuffer<float> tmpBuffer;
	
	void updateAudioInputsInternal() override;
	void updateAudioOutputsInternal() override;

	MixerItem * getMixerItem(int inputIndex, int outputIndex);

	void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;
	
	var getJSONData() override;
	void loadJSONDataItemInternal(var data) override;

	String getTypeString() const override { return getTypeStringStatic(); }
	static const String getTypeStringStatic() { return "Mixer"; }

	BaseNodeViewUI* createViewUI() override;
};
