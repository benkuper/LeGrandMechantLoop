/*
  ==============================================================================

    SpatNodeViewUI.cpp
    Created: 15 Nov 2020 11:41:00pm
    Author:  bkupe

  ==============================================================================
*/

#include "SpatNodeViewUI.h"

SpatNodeViewUI::SpatNodeViewUI(GenericAudioNode<SpatProcessor>* n) :
    GenericAudioNodeViewUI(n),
    spatView(processor)
{
    addAndMakeVisible(&spatView);
}

SpatNodeViewUI::~SpatNodeViewUI()
{
}

void SpatNodeViewUI::nodeOutputsChanged()
{
    spatView.updateItemsUI();
}


void SpatNodeViewUI::resizedInternalContentNode(Rectangle<int> &r)
{
    float minSize = jmin(r.getWidth(), r.getHeight());
    spatView.setBounds(r.withSizeKeepingCentre(minSize, minSize).reduced(4));
}

void SpatNodeViewUI::controllableFeedbackUpdateInternal(Controllable* c)
{
    GenericAudioNodeViewUI::controllableFeedbackUpdateInternal(c);
    if (c == processor->spatPosition || c == processor->circleRadius || c == processor->circleAngle || c == processor->spatRadius)
    {
        spatView.resized();
    }
    else if (SpatItem* si = c->getParentAs<SpatItem>())
    {
        spatView.placeItem(spatView.getUIForItem(si));
    }
}

