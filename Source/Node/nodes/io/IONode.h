/*
  ==============================================================================

	IONode.h
	Created: 15 Nov 2020 8:42:54am
	Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "../../NodeProcessor.h"

class IOProcessor :
	public GenericNodeProcessor
{
public:
	IOProcessor(Node* n, bool isInput);
	virtual ~IOProcessor() {}


	bool isInput;
	ControllableContainer rmsCC;
	ControllableContainer gainCC;
	var gainGhostData;

	void updateInputsFromNodeInternal() override;
	void updateOutputsFromNodeInternal() override;
	void updateIOFromNode();

	void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;
	void processBlockBypassed(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

	static const String getTypeStringStatic() { return "Audio IO"; }

	var getJSONData() override;
	void loadJSONDataInternal(var data) override;

	NodeViewUI* createNodeViewUI() override;
};

class AudioInputProcessor :
	public IOProcessor
{
public:
	AudioInputProcessor(Node* node) : IOProcessor(node, true) {}
	static const String getTypeStringStatic() { return "Audio Input"; }

	void updatePlayConfig() override;
};

class AudioOutputProcessor :
	public IOProcessor
{
public:
	AudioOutputProcessor(Node* node) : IOProcessor(node, false) {}
	static const String getTypeStringStatic() { return "Audio Output"; }

	void updatePlayConfig() override;

};