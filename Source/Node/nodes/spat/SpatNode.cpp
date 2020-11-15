/*
  ==============================================================================

    SpatNode.cpp
    Created: 15 Nov 2020 8:42:35am
    Author:  bkupe

  ==============================================================================
*/

#include "SpatNode.h"
#include "ui/SpatNodeViewUI.h"

SpatProcessor::SpatProcessor(Node* node) :
    GenericNodeAudioProcessor(node)
{
}

void SpatProcessor::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
}

NodeViewUI* SpatProcessor::createNodeViewUI()
{
    return new SpatNodeViewUI((GenericAudioNode<SpatProcessor>*)nodeRef.get());
}
