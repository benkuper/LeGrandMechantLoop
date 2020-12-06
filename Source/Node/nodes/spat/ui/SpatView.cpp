/*
  ==============================================================================

	SpatView.cpp
	Created: 21 Nov 2020 6:32:59pm
	Author:  bkupe

  ==============================================================================
*/

#include "SpatView.h"
#include "../SpatNode.h"

SpatView::SpatView(SpatNode* node) :
	node(node)
{
	updateItemsUI();
	addAndMakeVisible(&target);

	setInterceptsMouseClicks(true, true);

	removeMouseListener(this);
	addMouseListener(this, true);
	setDisableDefaultMouseEvents(true);
}

SpatView::~SpatView()
{
}

void SpatView::paint(Graphics& g)
{
	g.setColour(BG_COLOR.darker(.1f));
	g.fillRoundedRectangle(getLocalBounds().toFloat(), 2);

	g.setColour(BG_COLOR.brighter(.2f));
	g.drawRoundedRectangle(getLocalBounds().reduced(1).toFloat(), 2, 1);
}

void SpatView::resized()
{
	Point<int> centre = getLocalBounds().getRelativePoint(node->spatPosition->x, node->spatPosition->y);
	target.setBounds(Rectangle<int>(30, 30).withCentre(centre));

	for (auto& s : itemsUI)
	{
		placeItem(s);
	}
}

void SpatView::updateItemsUI()
{
	int numOutputs = node->spatCC.controllableContainers.size();
	while (itemsUI.size() > numOutputs)
	{
		removeChildComponent(itemsUI[itemsUI.size()]);
		itemsUI.removeLast();
	}

	while (itemsUI.size() < numOutputs)
	{
		SpatItemUI* si = new SpatItemUI((SpatItem*)node->spatCC.controllableContainers[itemsUI.size()].get());
		addAndMakeVisible(si);
		itemsUI.add(si);
	}

	target.toFront(false);
	resized();
}

void SpatView::mouseDown(const MouseEvent& e)
{
	if (SpatTarget* st = dynamic_cast<SpatTarget*>(e.eventComponent))
	{
		posAtMouseDown = node->spatPosition->getPoint();
	}
}

void SpatView::mouseDrag(const MouseEvent& e)
{

	Point<float> pos = getMouseXYRelative().toFloat() / Point<float>(getWidth(), getHeight());
	if (SpatItemUI* si = dynamic_cast<SpatItemUI*>(e.eventComponent))
	{
		if (node->spatMode->getValueDataAsEnum<SpatNode::SpatMode>() == SpatNode::FREE)
		{
			si->item->position->setPoint(pos);
		}
	}
	else if (SpatTarget* st = dynamic_cast<SpatTarget*>(e.eventComponent))
	{
		node->spatPosition->setPoint(pos);
	}
}

void SpatView::mouseUp(const MouseEvent& e)
{

	if (SpatItemUI* si = dynamic_cast<SpatItemUI*>(e.eventComponent))
	{
		if (node->spatMode->getValueDataAsEnum<SpatNode::SpatMode>() == SpatNode::FREE)
		{
			if (si->posAtMouseDown != si->item->position->getPoint())
			{
				si->item->position->setUndoablePoint(si->posAtMouseDown, si->item->position->getPoint());
			}
		}
	}
	else if (SpatTarget* st = dynamic_cast<SpatTarget*>(e.eventComponent))
	{
		node->spatPosition->setUndoablePoint(posAtMouseDown, node->spatPosition->getPoint());
	}
}


void SpatView::placeItem(SpatItemUI* ui)
{
	if (ui == nullptr) return;

	float size = ui->item->radius->floatValue() * 2;
	Rectangle<int> bounds = getLocalBounds().getProportion(Rectangle<float>().withSize(size, size).withCentre(ui->item->position->getPoint()));
	ui->setBounds(bounds);
}

SpatItemUI* SpatView::getUIForItem(SpatItem* item)
{
	for (auto& ui : itemsUI) if (ui->item == item) return ui;
	return nullptr;
}

void SpatView::SpatTarget::paint(Graphics& g)
{
	g.setColour(BLUE_COLOR.brighter(isMouseOverOrDragging() ? .3f : 0));
	g.fillEllipse(getLocalBounds().reduced(5).toFloat());
}
