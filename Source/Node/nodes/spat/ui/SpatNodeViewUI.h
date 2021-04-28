/*
  ==============================================================================

    SpatNodeViewUI.h
    Created: 15 Nov 2020 11:41:00pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

class SpatNodeViewUI :
    public NodeViewUI<SpatNode>
{
public:
    SpatNodeViewUI(SpatNode * n);
    ~SpatNodeViewUI();

    SpatView spatView;

    void nodeOutputsChanged() override;

    void resizedInternalContentNode(Rectangle<int> &r) override;
    void controllableFeedbackUpdateInternal(Controllable* c) override;
};