/*
  ==============================================================================

    LooperNodeViewUI.h
    Created: 15 Nov 2020 8:43:15am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "../LooperNode.h"
#include "../../../ui/NodeViewUI.h"
#include "Common/MIDI/ui/MIDIDeviceParameterUI.h"

class LooperTrackUI;

class LooperNodeViewUI :
    public NodeViewUI<LooperNode>
{
public:
    LooperNodeViewUI(LooperNode * n);
    ~LooperNodeViewUI();

    void updateTracksUI();

    std::unique_ptr<MIDIDeviceParameterUI> midiParamUI;
    OwnedArray<LooperTrackUI> tracksUI;

    void controllableFeedbackUpdateInternal(Controllable* c) override;

    void resizedInternalHeader(Rectangle<int>& r) override;
    void resizedInternalContentNode(Rectangle<int> &r) override;
};
