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

    trackOutputMode = addEnumParameter("Output Mode", "How to output the channels");
    trackOutputMode->addOption("Mixed only", MIXED_ONLY)->addOption("Tracks only", SEPARATE_ONLY)->addOption("Mixed and tracks", ALL);

    monitorMode = addEnumParameter("Monitor Mode", "How to monitor");
    monitorMode->addOption("Off", OFF)->addOption("Always", ALWAYS)->addOption("Armed track", ARMED_TRACK)->addOption("When recording", RECORDING_ONLY);

    node->setAudioInputs(numChannelsPerTrack->intValue());
    updateOutTracks();
}

void LooperProcessor::updateOutTracks()
{
    TrackOutputMode m = trackOutputMode->getValueDataAsEnum<TrackOutputMode>();

    StringArray s;

    if (m == MIXED_ONLY || m == ALL)
    {
        for (int i = 0; i < numChannelsPerTrack->intValue(); i++)
        {
            s.add("Mix " + String(i + 1));
        }
    }
    

    if (m == SEPARATE_ONLY || m == ALL)
    {
        for (int i = 0; i < numTracks->intValue(); i++)
        {
            s.add("Track " + String(i + 1));
        }
    }

    nodeRef->setAudioOutputs(s);
}

void LooperProcessor::onContainerParameterChanged(Parameter* p)
{
    if (nodeRef.wasObjectDeleted()) return;

    if (p == numTracks || p == trackOutputMode)
    {
        updateOutTracks();
    }
    else if (p == numChannelsPerTrack)
    {
        nodeRef->setAudioInputs(numChannelsPerTrack->intValue());

        TrackOutputMode tom = trackOutputMode->getValueDataAsEnum<TrackOutputMode>();
        if (tom == MIXED_ONLY || tom == ALL) updateOutTracks();
    }
}

void LooperProcessor::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    TrackOutputMode tom = trackOutputMode->getValueDataAsEnum<TrackOutputMode>();
    MonitorMode mm = monitorMode->getValueDataAsEnum<MonitorMode>();

    bool shouldClearBuffer = false;
    if (mm == OFF || tom == SEPARATE_ONLY) shouldClearBuffer = true;

    if (shouldClearBuffer) buffer.clear();
    
    if(tom == SEPARATE_ONLY || tom == ALL)
    {
        //tracks
    }
    
    if (tom == MIXED_ONLY || tom == ALL)
    {
        //mix
    }
}

NodeViewUI* LooperProcessor::createNodeViewUI()
{
    return new LooperNodeViewUI((GenericAudioNode<LooperProcessor>*)nodeRef.get());
}
