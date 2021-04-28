/*
  ==============================================================================

    SpatNodeViewUI.cpp
    Created: 15 Nov 2020 11:41:00pm
    Author:  bkupe

  ==============================================================================
*/

SpatNodeViewUI::SpatNodeViewUI(SpatNode * n) :
    NodeViewUI(n),
    spatView(n)
{
    addAndMakeVisible(&spatView);
    contentComponents.add(&spatView);
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
    NodeViewUI::controllableFeedbackUpdateInternal(c);
    if (c == node->spatPosition || c == node->circleRadius || c == node->circleAngle || c == node->spatRadius)
    {
        spatView.resized();
    }
    else if (SpatItem* si = c->getParentAs<SpatItem>())
    {
        spatView.placeItem(spatView.getUIForItem(si));
    }
}

