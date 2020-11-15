/*
  ==============================================================================

    MixerNode.cpp
    Created: 15 Nov 2020 8:42:42am
    Author:  bkupe

  ==============================================================================
*/

#include "MixerNode.h"
#include "ui/MixerNodeViewUI.h"

MixerProcessor::MixerProcessor(Node* node) :
    GenericNodeAudioProcessor(node)
{
}


void MixerProcessor::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
}

NodeViewUI* MixerProcessor::createNodeViewUI()
{
    return new MixerNodeViewUI((GenericAudioNode<MixerProcessor>*)nodeRef.get());
}
