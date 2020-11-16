/*
  ==============================================================================

    NodeAudioProcessor.cpp
    Created: 15 Nov 2020 10:34:19pm
    Author:  bkupe

  ==============================================================================
*/

#include "NodeAudioProcessor.h"
#include "Node.h"
#include "ui/NodeViewUI.h"

NodeAudioProcessor::NodeAudioProcessor(Node* n) : 
    ControllableContainer("Processor"),
    nodeRef(n) 
{
    hideEditorHeader = true;
}


/* AudioProcessor */

const String NodeAudioProcessor::getName() const {
    return nodeRef.wasObjectDeleted() ? "[Deleted]" : nodeRef->niceName; 
}

void NodeAudioProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    if (nodeRef == nullptr || nodeRef.wasObjectDeleted())
    {
        buffer.clear();
        return;
    }

    if (!nodeRef->enabled->boolValue())
    {
        processBlockBypassed(buffer, midiMessages);
        return;
    }

    processBlockInternal(buffer, midiMessages);
}

GenericNodeAudioProcessor::GenericNodeAudioProcessor(Node* n) :
    NodeAudioProcessor(n)
{
}

NodeViewUI* GenericNodeAudioProcessor::createNodeViewUI()
{
    return new NodeViewUI(nodeRef.get()); 
}
