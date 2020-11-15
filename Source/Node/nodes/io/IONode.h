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
	IOProcessor(Node* n) : GenericNodeAudioProcessor(n) {}
	virtual ~IOProcessor() {}
};


class AudioInputProcessor :
	public IOProcessor
{
public:
	AudioInputProcessor(Node* node);
	~AudioInputProcessor() {}

	void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;
	static const String getTypeStringStatic() { return "Audio Input"; }

	NodeViewUI* createNodeViewUI() override;
};


class AudioOutputProcessor :
	public IOProcessor
{
public:
	AudioOutputProcessor(Node* node);
	~AudioOutputProcessor() {}

	void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;
	static const String getTypeStringStatic() { return "Audio Output"; }

	NodeViewUI* createNodeViewUI() override;
};