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
    spatView(audioNode->processor)
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
    if (c == audioNode->processor->spatPosition || c == audioNode->processor->circleRadius || c == audioNode->processor->circleAngle || c == audioNode->processor->spatRadius)
    {
        spatView.resized();
    }
    else if (SpatItem* si = c->getParentAs<SpatItem>())
    {
        spatView.placeItem(spatView.getUIForItem(si));
    }
}

