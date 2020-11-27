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

	if (item->baseProcessor->outRMS != nullptr)
	{
		outRMSUI.reset(new RMSSliderUI(item->baseProcessor->outRMS));
		addAndMakeVisible(outRMSUI.get());

	}

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
	if (item->baseProcessor->hasInput)
	{
		if (inAudioConnector == nullptr)
		{
			inAudioConnector.reset(new NodeConnector(this, true));
			addAndMakeVisible(inAudioConnector.get());
			resized();
		}
		else
		{
			inAudioConnector->update();
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
	if (item->baseProcessor->hasOutput)
	{
		if (outAudioConnector == nullptr)
		{
			outAudioConnector.reset(new NodeConnector(this, false));
			addAndMakeVisible(outAudioConnector.get());
			resized();
		}
		else
		{
			outAudioConnector->update();
		}
	}
	else if (outAudioConnector != nullptr)
	{
		removeChildComponent(outAudioConnector.get());
		outAudioConnector.reset();
	}
}

void NodeViewUI::paintOverChildren(Graphics& g)
{
	BaseItemUI::paintOverChildren(g);
	if (!item->enabled->boolValue())
	{
		if (inAudioConnector != nullptr && outAudioConnector != nullptr)
		{
			g.setColour(BLUE_COLOR);
			g.drawLine(Line<int>(inAudioConnector->getBounds().getCentre(), outAudioConnector->getBounds().getCentre()).toFloat(), 2);
		}
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
	if (outRMSUI != nullptr) outRMSUI->setBounds(r.removeFromRight(10).reduced(2));
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
