/*
  ==============================================================================

	SpatItemUI.cpp
	Created: 21 Nov 2020 6:32:53pm
	Author:  bkupe

  ==============================================================================
*/

#include "Node/NodeIncludes.h"

SpatItemUI::SpatItemUI(SpatItem* item) :
	BaseItemMinimalUI(item),
	item(item)
{
	setSize(30, 30);
	dragAndDropEnabled = false;
	setInterceptsMouseClicks(true, false);
}

SpatItemUI::~SpatItemUI()
{
}

void SpatItemUI::mouseDown(const MouseEvent& e)
{
	posAtMouseDown = item->position->getPoint();
}

bool SpatItemUI::hitTest(int x, int y)
{
	Rectangle<int> center = getLocalBounds().withSizeKeepingCentre(30, 30);
	return center.contains(x, y);
}

void SpatItemUI::paint(Graphics& g)
{
	if (inspectable.wasObjectDeleted()) return;

	Colour c = item->itemColor != nullptr ? item->itemColor->getColor() : NORMAL_COLOR;

	Rectangle<float> center = getLocalBounds().withSizeKeepingCentre(20, 20).toFloat();
	Colour cc = c.brighter(isMouseOverOrDragging() ? .3f : 0);
	g.setColour(c.withAlpha(.6f));
	g.fillEllipse(center);

	Colour tc = c.getPerceivedBrightness() > .5f ? c.darker(1.2f) : c.brighter(1.2f);
	tc = tc.withMultipliedSaturation(.2f);
	g.drawText(String(item->index), center, Justification::centred);
}


void SpatTargetUI::paint(Graphics& g)
{
	if (inspectable.wasObjectDeleted()) return;

	for (int i = 0; i < target->weights.size(); i++)
	{
		float size = target->weights[i]->floatValue() * getWidth();
		Colour c = target->colors[i];
		g.setColour(c.withAlpha(.4f));
		g.fillEllipse(getLocalBounds().toFloat().withSizeKeepingCentre(size, size));


		g.setColour(NORMAL_COLOR.brighter().withAlpha(.6f));
		g.drawEllipse(getLocalBounds().toFloat(), 1);
	}


	SpatItemUI::paint(g);
}

void SpatTargetUI::controllableFeedbackUpdateInternal(Controllable* c)
{
	SpatItemUI::controllableFeedbackUpdateInternal(c);

	if (target->weights.contains((FloatParameter*)c)) repaint(); //could optimize with common timer
}
