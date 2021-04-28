/*
  ==============================================================================

	IONode.h
	Created: 15 Nov 2020 8:42:54am
	Author:  bkupe

  ==============================================================================
*/

#pragma once

class IONode :
	public Node
{
public:
	IONode(StringRef name, var params = var(), bool isInput = true);
	virtual ~IONode() {}

	bool isInput;
	ControllableContainer channelsCC;
	var channelsGhostData;
	bool isRoot;

	int realNumInputs;
	int realNumOutputs;

	int ghostNumChannels; //for root nodes

	void setIsRoot(bool value);

	virtual void setAudioInputs(const StringArray& inputNames, bool updateConfig = true) override;
	virtual void setAudioOutputs(const StringArray& outputNames, bool updateConfig = true) override;

	void updateAudioInputsInternal() override;
	void updateAudioOutputsInternal() override;
	void updateIO();

	void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;
	void processBlockBypassed(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

	var getJSONData() override;
	void loadJSONDataItemInternal(var data) override;

	BaseNodeViewUI* createViewUI() override;
};

class AudioInputNode :
	public IONode
{
public:
	AudioInputNode(var params = var()) : IONode(getTypeString(), params, true) {}

	String getTypeString() const override { return getTypeStringStatic(); }
	static const String getTypeStringStatic() { return "Audio In"; }

	void updatePlayConfigInternal() override;
};

class AudioOutputNode :
	public IONode
{
public:
	AudioOutputNode(var params = var()) : IONode(getTypeString(), params, false) {}
	
	String getTypeString() const override { return getTypeStringStatic(); }
	static const String getTypeStringStatic() { return "Audio Out"; }

	void updatePlayConfigInternal() override;
};