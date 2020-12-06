/*
  ==============================================================================

	IONode.h
	Created: 15 Nov 2020 8:42:54am
	Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "../../Node.h"

class IOChannel :
	public ControllableContainer
{
public:
	IOChannel(StringRef name);
	~IOChannel() {}
	FloatParameter* gain;
	FloatParameter* rms;
	BoolParameter* active;
};

class IONode :
	public Node
{
public:
	IONode(var params = var(), bool isInput = true);
	virtual ~IONode() {}


	bool isInput;
	ControllableContainer channelsCC;
	var gainGhostData;

	void updateAudioInputsInternal() override;
	void updateAudioOutputsInternal() override;
	void updateIO();

	void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;
	void processBlockBypassed(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

	var getJSONData() override;
	void loadJSONDataInternal(var data) override;

	BaseNodeViewUI* createViewUI() override;
};

class AudioInputNode :
	public IONode
{
public:
	AudioInputNode(var params = var()) : IONode(params, true) {}

	String getTypeString() const override { return getTypeStringStatic(); }
	static const String getTypeStringStatic() { return "Audio Input"; }

	void updatePlayConfigInternal() override;
};

class AudioOutputNode :
	public IONode
{
public:
	AudioOutputNode(var params = var()) : IONode(params, false) {}
	
	String getTypeString() const override { return getTypeStringStatic(); }
	static const String getTypeStringStatic() { return "Audio Output"; }

	void updatePlayConfigInternal() override;
};