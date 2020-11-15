/*
  ==============================================================================

    LooperNode.h
    Created: 15 Nov 2020 8:43:03am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "../../NodeAudioProcessor.h"

class LooperProcessor :
        public GenericNodeAudioProcessor
{
public:
    LooperProcessor(Node * node);
    ~LooperProcessor() {}

    void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;
    static String getTypeStringStatic() { return "Looper"; }
  
    NodeViewUI* createNodeViewUI() override;
};