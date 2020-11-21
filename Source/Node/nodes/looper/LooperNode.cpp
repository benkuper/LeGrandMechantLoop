/*
  ==============================================================================

    LooperNode.cpp
    Created: 15 Nov 2020 8:43:03am
    Author:  bkupe

  ==============================================================================
*/

#include "LooperNode.h"
#include "ui/LooperNodeViewUI.h"
#include "LooperTrack.h"

LooperProcessor::LooperProcessor(Node* node) :
    GenericNodeAudioProcessor(node),
    tracksCC("Tracks")
{
    numTracks = addIntParameter("Track Count", "Number of tracks to use for this looper", 8, 1, 32);
    numChannelsPerTrack = addIntParameter("Channel Per Track", "Number of channel to use for each track", 1, 1, 8);

    trackOutputMode = addEnumParameter("Output Mode", "How to output the channels");
    trackOutputMode->addOption("Mixed only", MIXED_ONLY)->addOption("Tracks only", SEPARATE_ONLY)->addOption("Mixed and tracks", ALL);

    monitorMode = addEnumParameter("Monitor Mode", "How to monitor");
    monitorMode->addOption("Off", OFF)->addOption("Always", ALWAYS)->addOption("Armed track", ARMED_TRACK)->addOption("When recording", RECORDING_ONLY);

    addChildControllableContainer(&tracksCC);
    
    node->setAudioInputs(numChannelsPerTrack->intValue());
    updateOutTracks();
    updateLooperTracks();
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

void LooperProcessor::updateLooperTracks()
{
    GenericScopedLock<SpinLock> lock(processorLock);

    while (tracksCC.controllableContainers.size() > numTracks->intValue())
    {
        tracksCC.removeChildControllableContainer(tracksCC.controllableContainers[tracksCC.controllableContainers.size() - 1]);
    }

    Array<ControllableContainer *> tracksToAdd;
    for (int i = tracksCC.controllableContainers.size(); i < numTracks->intValue(); i++)
    {
        tracksCC.addChildControllableContainer(new LooperTrack(i, numChannelsPerTrack->intValue()), true);
    }
}

void LooperProcessor::onContainerParameterChanged(Parameter* p)
{
    if (nodeRef.wasObjectDeleted()) return;

    if (p == numTracks || p == trackOutputMode)
    {
        updateOutTracks();
        updateLooperTracks();
    }
    else if (p == numChannelsPerTrack)
    {
        nodeRef->setAudioInputs(numChannelsPerTrack->intValue());

        TrackOutputMode tom = trackOutputMode->getValueDataAsEnum<TrackOutputMode>();
        if (tom == MIXED_ONLY || tom == ALL) updateOutTracks();

        for (auto& cc : tracksCC.controllableContainers)
        {
            ((LooperTrack*)cc.get())->setNumChannels(numChannelsPerTrack->intValue());
        }
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
