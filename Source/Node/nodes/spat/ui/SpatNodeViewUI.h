/*
  ==============================================================================

    SpatNodeViewUI.h
    Created: 15 Nov 2020 11:41:00pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "../SpatNode.h"
#include "../../../ui/NodeViewUI.h"
#include "SpatView.h"

class SpatNodeViewUI :
    public GenericAudioNodeViewUI<SpatProcessor>
{
public:
    SpatNodeViewUI(GenericAudioNode<SpatProcessor>* n);
    ~SpatNodeViewUI();

    SpatView spatView;

    void nodeOutputsChanged() override;

    void resizedInternalContentNode(Rectangle<int> &r) override;
    void controllableFeedbackUpdateInternal(Controllable* c) override;
};