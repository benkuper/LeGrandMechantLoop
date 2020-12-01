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

class LooperTrackUI;

class LooperNodeViewUI :
    public GenericNodeViewUI<LooperProcessor>
{
public:
    LooperNodeViewUI(GenericNode<LooperProcessor>* n);
    ~LooperNodeViewUI();

    void updateTracksUI();

    OwnedArray<LooperTrackUI> tracksUI;

    void controllableFeedbackUpdateInternal(Controllable* c) override;

    void resizedInternalContentNode(Rectangle<int> &r) override;
};
