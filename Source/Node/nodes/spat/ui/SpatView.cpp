/*
  ==============================================================================

	SpatView.cpp
	Created: 21 Nov 2020 6:32:59pm
	Author:  bkupe

  ==============================================================================
*/

#include "Node/NodeIncludes.h"

SpatView::SpatView(SpatNode* node) :
	node(node)
{
	updateItemsUI();

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

	for (auto& s : targetsUI)
	{
		placeTarget(s);
	}

	for (auto& s : sourcesUI)
	{
		placeSource(s);
	}
}

void SpatView::updateItemsUI()
{
	int numInputs = node->sources.items.size();


	while (sourcesUI.size() > numInputs)
	{
		removeChildComponent(sourcesUI[sourcesUI.size()]);
		sourcesUI.removeLast();
	}

	while (sourcesUI.size() < numInputs)
	{
		SpatSourceUI* ss = new SpatSourceUI(node->sources.items[sourcesUI.size()]);
		addAndMakeVisible(ss);
		sourcesUI.add(ss);
	}


	int numOutputs = node->targets.items.size();

	while (targetsUI.size() > numOutputs)
	{
		removeChildComponent(targetsUI[targetsUI.size()]);
		targetsUI.removeLast();
	}

	while (targetsUI.size() < numOutputs)
	{
		SpatTargetUI* si = new SpatTargetUI(node->targets.items[targetsUI.size()]);
		addAndMakeVisible(si);
		targetsUI.add(si);
	}


	for (auto& s : sourcesUI)
	{
		s->toFront(false);
	}

	resized();
}

void SpatView::mouseDown(const MouseEvent& e)
{
	if (SpatItemUI* si = dynamic_cast<SpatItemUI*>(e.eventComponent))
	{
		si->posAtMouseDown = si->item->position->getPoint();
	}
}

void SpatView::mouseDrag(const MouseEvent& e)
{
	Point<float> pos = getMouseXYRelative().toFloat() / Point<float>(getWidth(), getHeight());
	pos.y = 1 - pos.y;

	if (SpatItemUI* si = dynamic_cast<SpatItemUI*>(e.eventComponent))
	{
		if (SpatSource* ss = dynamic_cast<SpatSource*>(e.eventComponent))
		{
			SpatSource::ControlMode m = ss->controlMode->getValueDataAsEnum<SpatSource::ControlMode>();
			if (m != SpatSource::FREE_2D) return;

		}
		else if (SpatTarget* ss = dynamic_cast<SpatTarget*>(e.eventComponent))
		{
			if (node->spatMode->getValueDataAsEnum<SpatNode::SpatMode>() != SpatNode::FREE) return;
		}

		si->item->position->setPoint(pos);
	}
}

void SpatView::mouseUp(const MouseEvent& e)
{

	if (SpatItemUI* si = dynamic_cast<SpatItemUI*>(e.eventComponent))
	{
		if (SpatSource* ss = dynamic_cast<SpatSource*>(e.eventComponent))
		{
			SpatSource::ControlMode m = ss->controlMode->getValueDataAsEnum<SpatSource::ControlMode>();
			if (m != SpatSource::FREE_2D) return;

		}
		else if (SpatTarget* ss = dynamic_cast<SpatTarget*>(e.eventComponent))
		{
			if (node->spatMode->getValueDataAsEnum<SpatNode::SpatMode>() != SpatNode::FREE) return;
		}

		si->item->position->setUndoablePoint(si->posAtMouseDown, si->item->position->getPoint());
	}
}

void SpatView::placeSource(SpatSourceUI* ui)
{
	if (ui == nullptr) return;
	Point<float> pos = ui->source->position->getPoint();
	ui->setCentrePosition(getLocalBounds().getRelativePoint(pos.x, 1 - pos.y));
}

void SpatView::placeTarget(SpatTargetUI* ui)
{
	if (ui == nullptr) return;

	float size = ui->item->radius->floatValue() * 2;
	Point<float> pos = ui->item->position->getPoint();
	pos.y = 1 - pos.y;
	Rectangle<int> bounds = getLocalBounds().getProportion(Rectangle<float>().withSize(size, size).withCentre(pos));
	ui->setBounds(bounds);
}

SpatTargetUI* SpatView::getUIForTarget(SpatTarget* item)
{
	for (auto& ui : targetsUI) if (ui->item == item) return ui;
	return nullptr;
}

SpatSourceUI* SpatView::getUIForSource(SpatSource* source)
{
	for (auto& ui : sourcesUI) if (ui->source == source) return ui;
	return nullptr;
}
