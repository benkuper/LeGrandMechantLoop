/*
  ==============================================================================

    VSTNode.h
    Created: 15 Nov 2020 8:42:29am
    Author:  bkupe

  ==============================================================================
*/

#pragma once
#include "../../NodeAudioProcessor.h"

class VSTProcessor :
	public GenericNodeAudioProcessor
{
public:
	VSTProcessor(Node* node);
	~VSTProcessor() {}

	void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;
	static const String getTypeStringStatic() { return "VST"; }

	NodeViewUI* createNodeViewUI() override;
};
