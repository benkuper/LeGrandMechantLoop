/*
  ==============================================================================

	NodeViewUI.cpp
	Created: 15 Nov 2020 9:26:57am
	Author:  bkupe

  ==============================================================================
*/

#include "NodeViewUI.h"
#include "../Connection/ui/NodeConnector.h"

NodeViewUI::NodeViewUI(Node* node) :
	BaseItemUI(node, Direction::ALL)
{
	dragAndDropEnabled = true;
	autoHideWhenDragging = false;
	drawEmptyDragIcon = true;

	updateInputConnectors();
	updateOutputConnectors();
	
	item->addAsyncNodeListener(this);

}

NodeViewUI::~NodeViewUI()
{
	if(!inspectable.wasObjectDeleted()) item->removeAsyncNodeListener(this);
}

void NodeViewUI::updateInputConnectors()
{
	bool showInConnector = item->hasAudioInput();
	if (showInConnector)
	{
		if (inAudioConnector == nullptr)
		{
			inAudioConnector.reset(new NodeConnector(this, true));
			addAndMakeVisible(inAudioConnector.get());
			resized();
		}
		else
		{
			inAudioConnector->updateTooltip();
		}

	}
	else if (inAudioConnector != nullptr)
	{
		removeChildComponent(inAudioConnector.get());
		inAudioConnector.reset();
	}

}

void NodeViewUI::updateOutputConnectors()
{
	bool showOutConnector = item->hasAudioOutput();
	if (showOutConnector)
	{
		if (outAudioConnector == nullptr)
		{
			outAudioConnector.reset(new NodeConnector(this, false));
			addAndMakeVisible(outAudioConnector.get());
			resized();
		}
		else
		{
			outAudioConnector->updateTooltip();
		}
	}
	else if (outAudioConnector != nullptr)
	{
		removeChildComponent(outAudioConnector.get());
		outAudioConnector.reset();
	}
}

void NodeViewUI::resized()
{
	BaseItemUI::resized();

	Rectangle<int> inR = getLocalBounds().removeFromLeft(10);
	Rectangle<int> outR = getLocalBounds().removeFromRight(10);

	if (inAudioConnector != nullptr) inAudioConnector->setBounds(inR.withHeight(inR.getWidth()).withCentre(Point<int>(inR.getCentreX(), jmin(inR.getCentreY(), 20))));
	if (outAudioConnector != nullptr) outAudioConnector->setBounds(outR.withHeight(outR.getWidth()).withCentre(Point<int>(outR.getCentreX(), jmin(outR.getCentreY(), 20))));
}

void NodeViewUI::resizedInternalContent(Rectangle<int>& r)
{
	BaseItemUI::resizedInternalContent(r);
	resizedInternalContentNode(r);
}

Rectangle<int> NodeViewUI::getMainBounds()
{
	return getLocalBounds().reduced(10, 0);
}

void NodeViewUI::nodeInputsChanged()
{
	updateInputConnectors();
}

void NodeViewUI::nodeOutputsChanged()
{
	updateOutputConnectors();
}

void NodeViewUI::newMessage(const Node::NodeEvent& e)
{
	if (e.type == e.INPUTS_CHANGED) nodeInputsChanged();
	else if (e.type == e.OUTPUTS_CHANGED) nodeOutputsChanged();
}
