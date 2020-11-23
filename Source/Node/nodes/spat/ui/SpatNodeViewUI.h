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
#include "SpatItemUI.h"

class SpatNodeViewUI :
    public GenericAudioNodeViewUI<SpatProcessor>
{
public:
    SpatNodeViewUI(GenericAudioNode<SpatProcessor>* n);
    ~SpatNodeViewUI();

    OwnedArray<SpatItemUI> itemsUI;

    Rectangle<int> spatRect;
    Rectangle<int> posRect;

    void updateItemsUI();

    void nodeOutputsChanged() override;

    void paint(Graphics& g) override;
    void resizedInternalContentNode(Rectangle<int> &r) override;

    void placeItem(SpatItemUI* ui);
    SpatItemUI * getUIForItem(SpatItem* item);

    void mouseDrag(const MouseEvent &e) override;

    void mouseUp(const MouseEvent& e) override;

    void controllableFeedbackUpdateInternal(Controllable* c) override;
};