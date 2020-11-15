/*
  ==============================================================================

    SpatNode.h
    Created: 15 Nov 2020 8:42:35am
    Author:  bkupe

  ==============================================================================
*/

#pragma once
#include "../../NodeAudioProcessor.h"

class SpatProcessor :
	public GenericNodeAudioProcessor
{
public:
	SpatProcessor(Node* node);
	~SpatProcessor() {}

	void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;
	static const String getTypeStringStatic() { return "Spat"; }

	NodeViewUI* createNodeViewUI() override;
};
