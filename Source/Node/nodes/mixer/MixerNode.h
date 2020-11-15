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

	void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;
	static const String getTypeStringStatic() { return "Mixer"; }

	NodeViewUI* createNodeViewUI() override;
};
