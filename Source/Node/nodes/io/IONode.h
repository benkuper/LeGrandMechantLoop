/*
  ==============================================================================

	IONode.h
	Created: 15 Nov 2020 8:42:54am
	Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "../../NodeAudioProcessor.h"

class IOProcessor :
	public GenericNodeAudioProcessor
{
public:
	IOProcessor(Node* n, bool isInput);
	virtual ~IOProcessor() {}


	bool isInput;
	ControllableContainer rmsCC;
	ControllableContainer gainCC;
	var gainGhostData;

	void updateInputsFromNode() override;
	void updateOutputsFromNode() override;
	void updateIOFromNode();

	void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

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
};

class AudioOutputProcessor :
	public IOProcessor
{
public:
	AudioOutputProcessor(Node* node) : IOProcessor(node, false) {}
	static const String getTypeStringStatic() { return "Audio Output"; }
};