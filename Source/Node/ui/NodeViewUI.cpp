/*
  ==============================================================================

	BaseNodeViewUI.cpp
	Created: 15 Nov 2020 9:26:57am
	Author:  bkupe

  ==============================================================================
*/

#include "NodeViewUI.h"
#include "../Connection/ui/NodeConnector.h"

BaseNodeViewUI::BaseNodeViewUI(Node* node) :
	BaseItemUI(node, Direction::ALL, true)
{
	dragAndDropEnabled = true;
	autoHideWhenDragging = false;
	drawEmptyDragIcon = true;

	if (item->outGain!= nullptr)
	{
		outGainUI.reset(item->outGain->createSlider());
		outGainUI->orientation = FloatSliderUI::VERTICAL;
		contentComponents.add(outGainUI.get());
		addAndMakeVisible(outGainUI.get());
	}

	if (item->outRMS != nullptr)
	{
		outRMSUI.reset(new RMSSliderUI(item->outRMS));
		contentComponents.add(outRMSUI.get());
		addAndMakeVisible(outRMSUI.get());
	}

	updateInputConnectors();
	updateOutputConnectors();
	
	item->addAsyncNodeListener(this);

}

BaseNodeViewUI::~BaseNodeViewUI()
{
	if(!inspectable.wasObjectDeleted()) item->removeAsyncNodeListener(this);
}

void BaseNodeViewUI::updateInputConnectors()
{
	//AUDIO
	if (item->hasAudioInput)
	{
		if (inAudioConnector == nullptr)
		{
			inAudioConnector.reset(new NodeConnector(this, true, NodeConnection::AUDIO));
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

	if (item->hasMIDIInput)
	{
		if (inMIDIConnector == nullptr)
		{
			inMIDIConnector.reset(new NodeConnector(this, true, NodeConnection::MIDI));
			addAndMakeVisible(inMIDIConnector.get());
			resized();
		}
		else
		{
			inMIDIConnector->update();
		}
	}
	else if (inMIDIConnector != nullptr)
	{
		removeChildComponent(inMIDIConnector.get());
		inMIDIConnector.reset();
	}
}

void BaseNodeViewUI::updateOutputConnectors()
{
	if (item->hasAudioOutput)
	{
		if (outAudioConnector == nullptr)
		{
			outAudioConnector.reset(new NodeConnector(this, false, NodeConnection::AUDIO));
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

	if (item->hasMIDIOutput)
	{
		if (outMIDIConnector == nullptr)
		{
			outMIDIConnector.reset(new NodeConnector(this, false, NodeConnection::MIDI));
			addAndMakeVisible(outMIDIConnector.get());
			resized();
		}
		else
		{
			outMIDIConnector->update();
		}
	}
	else if (outMIDIConnector != nullptr)
	{
		removeChildComponent(outMIDIConnector.get());
		outMIDIConnector.reset();
	}
}

void BaseNodeViewUI::paint(Graphics& g)
{
	BaseItemUI::paint(g);
	if (outRMSUI != nullptr || outGainUI != nullptr)
	{
		g.setColour(bgColor.darker(.2f));
		g.fillRoundedRectangle(outControlRect.expanded(1).toFloat(), 2);
	}
}

void BaseNodeViewUI::paintOverChildren(Graphics& g)
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

void BaseNodeViewUI::resized()
{
	BaseItemUI::resized();

	int w = 10;
	Rectangle<int> inR = getLocalBounds().removeFromLeft(w).reduced(0, 10);
	Rectangle<int> outR = getLocalBounds().removeFromRight(w).reduced(0, 10);

	if (inAudioConnector != nullptr)
	{
		inAudioConnector->setBounds(inR.removeFromTop(w));
		inR.removeFromTop(20);
	}

	if (outAudioConnector != nullptr)
	{
		outAudioConnector->setBounds(outR.removeFromTop(w));
		outR.removeFromTop(20);
	}

	if (inMIDIConnector != nullptr) inMIDIConnector->setBounds(inR.removeFromTop(w));
	if (outMIDIConnector != nullptr) outMIDIConnector->setBounds(outR.removeFromTop(w));
}

void BaseNodeViewUI::resizedInternalContent(Rectangle<int>& r)
{
	BaseItemUI::resizedInternalContent(r);

	outControlRect = Rectangle<int>(r);

	if (outRMSUI != nullptr) outRMSUI->setBounds(r.removeFromRight(10).reduced(2));
	if (outGainUI != nullptr) outGainUI->setBounds(r.removeFromRight(20).reduced(2));
	r.removeFromRight(2);
	outControlRect.setLeft(r.getRight());

	resizedInternalContentNode(r);
}

Rectangle<int> BaseNodeViewUI::getMainBounds()
{
	return getLocalBounds().reduced(10, 0);
}

void BaseNodeViewUI::nodeInputsChanged()
{
	updateInputConnectors();
}

void BaseNodeViewUI::nodeOutputsChanged()
{
	updateOutputConnectors();
}

void BaseNodeViewUI::newMessage(const Node::NodeEvent& e)
{
	if (e.type == e.INPUTS_CHANGED || e.type == e.MIDI_INPUT_CHANGED) nodeInputsChanged();
	else if (e.type == e.OUTPUTS_CHANGED || e.type == e.MIDI_OUTPUT_CHANGED) nodeOutputsChanged();
}
