/*
  ==============================================================================

    LooperNode.cpp
    Created: 15 Nov 2020 8:43:03am
    Author:  bkupe

  ==============================================================================
*/

#include "LooperNode.h"
#include "ui/LooperNodeViewUI.h"

LooperProcessor::LooperProcessor(Node* node) :
    GenericNodeAudioProcessor(node)
{
}

void LooperProcessor::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
}

NodeViewUI* LooperProcessor::createNodeViewUI()
{
    return new LooperNodeViewUI((GenericAudioNode<LooperProcessor>*)nodeRef.get());
}
