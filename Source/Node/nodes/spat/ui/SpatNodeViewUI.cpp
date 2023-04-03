/*
  ==============================================================================

	SpatNodeViewUI.cpp
	Created: 15 Nov 2020 11:41:00pm
	Author:  bkupe

  ==============================================================================
*/

#include "Node/NodeIncludes.h"

SpatNodeViewUI::SpatNodeViewUI(SpatNode* n) :
	NodeViewUI(n),
	spatView(n)
{
	addAndMakeVisible(&spatView);
	contentComponents.add(&spatView);

	spatView.resized();
}

SpatNodeViewUI::~SpatNodeViewUI()
{
}

void SpatNodeViewUI::nodeInputsChanged()
{
	spatView.updateItemsUI();
}

void SpatNodeViewUI::nodeOutputsChanged()
{
	spatView.updateItemsUI();
}


void SpatNodeViewUI::resizedInternalContentNode(Rectangle<int>& r)
{
	float minSize = jmin(r.getWidth(), r.getHeight());
	spatView.setBounds(r.withSizeKeepingCentre(minSize, minSize).reduced(4));
}

void SpatNodeViewUI::controllableFeedbackUpdateInternal(Controllable* c)
{
	NodeViewUI::controllableFeedbackUpdateInternal(c);
	if (c == node->circleRadius || c == node->circleAngle || c == node->spatRadius)
	{
		spatView.resized();
	}
	else if (SpatTarget* st = c->getParentAs<SpatTarget>())
	{
		if (c == st->position || c == st->radius) spatView.placeTarget(spatView.getUIForTarget(st));
	}
	else if (SpatSource* ss = c->getParentAs<SpatSource>())
	{
		if (c == ss->position) spatView.placeSource(spatView.getUIForSource(ss));
	}
}

