/*
  ==============================================================================

	MixerNode.h
	Created: 15 Nov 2020 8:42:42am
	Author:  bkupe

  ==============================================================================
*/

#pragma once

class MixerItem :
	public VolumeControl
{
public:
	MixerItem(int inputIndex, int outputIndex, bool hasRMS = false);
	~MixerItem();

	int inputIndex;
	int outputIndex;
};

class InputLineCC :
	public ControllableContainer
{
public:
	InputLineCC(int index);
	~InputLineCC();

	int index;

	Array<MixerItem*> mixerItems;
	IntParameter* exclusiveIndex;
	bool isSettingFromIndex;

	void onContainerParameterChanged(Parameter* p) override;
	void onControllableStateChanged(Controllable* p) override;
	void onControllableFeedbackUpdate(ControllableContainer* cc, Controllable* c) override;

	void updateExclusiveOutput();

	void setNumOutputs(int numOutputs);
};

class MixerNode :
	public Node
{
public:
	MixerNode(var params = var());
	~MixerNode() {}

	Array<InputLineCC*> inputLines;
	Array<VolumeControl *> mainOuts;

	BoolParameter* showOutputGains;
	BoolParameter* showOutputRMS;
	BoolParameter* showOutputActives;
	BoolParameter* showOutputExclusives;
	BoolParameter* showItemGains;
	BoolParameter* showItemActives;

	AudioBuffer<float> tmpBuffer;
	
	void updateAudioInputsInternal() override;
	void updateAudioOutputsInternal() override;

	MixerItem * getMixerItem(int inputIndex, int outputIndex);

	void reorderContainers();

	void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

	var getJSONData() override;
	void loadJSONDataItemInternal(var data) override;
	void afterLoadJSONDataInternal() override;

	String getTypeString() const override { return getTypeStringStatic(); }
	static const String getTypeStringStatic() { return "Mixer"; }

	BaseNodeViewUI* createViewUI() override;
};
