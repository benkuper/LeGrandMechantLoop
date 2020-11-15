/*
  ==============================================================================

    VSTNode.cpp
    Created: 15 Nov 2020 8:42:29am
    Author:  bkupe

  ==============================================================================
*/

#include "VSTNode.h"
#include "ui/VSTNodeViewUI.h"

VSTProcessor::VSTProcessor(Node* node) :
    GenericNodeAudioProcessor(node)
{
}

void VSTProcessor::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
}

NodeViewUI* VSTProcessor::createNodeViewUI()
{
    return new VSTNodeViewUI((GenericAudioNode<VSTProcessor>*)nodeRef.get());
}
