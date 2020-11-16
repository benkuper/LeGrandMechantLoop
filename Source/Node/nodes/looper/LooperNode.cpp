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
    numTracks = addIntParameter("Track Count", "Number of tracks to use for this looper", 8, 1, 32);
    numChannelsPerTrack = addIntParameter("Channel Per Track", "Number of channel to use for each track", 1, 1, 8);

    node->setAudioInputs(numChannelsPerTrack->intValue());
    updateAudioOutputs();
}

void LooperProcessor::updateAudioOutputs()
{
    StringArray s;
    for (int i = 0; i < numTracks->intValue(); i++) s.add("Track " + String(i + 1));
    nodeRef->setAudioOutputs(s);
}

void LooperProcessor::onContainerParameterChanged(Parameter* p)
{
    if (nodeRef.wasObjectDeleted()) return;

    if (p == numTracks)
    {
        updateAudioOutputs();
    }
    else if (p == numChannelsPerTrack)
    {
        nodeRef->setAudioInputs(numChannelsPerTrack->intValue());
    }
}

void LooperProcessor::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
}

NodeViewUI* LooperProcessor::createNodeViewUI()
{
    return new LooperNodeViewUI((GenericAudioNode<LooperProcessor>*)nodeRef.get());
}
