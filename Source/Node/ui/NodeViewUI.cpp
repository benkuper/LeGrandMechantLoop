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
	showRemoveBT = false;

	viewFilterUpdated(); //force refresh

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
	if (!item->miniMode->boolValue() && outControlUI != nullptr)
	{
		g.setColour(bgColor.darker(.3f));
		g.fillRoundedRectangle(outControlRect.expanded(1).toFloat(), 2);
		g.setColour(bgColor.darker(.6f));
		g.drawRoundedRectangle(outControlRect.expanded(1).toFloat(), 2, 1);
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
	r.removeFromLeft(2);

	outControlRect = Rectangle<int>(r);

	if (outControlUI != nullptr) outControlUI->setBounds(r.removeFromRight(30).reduced(2));
	
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

void BaseNodeViewUI::viewFilterUpdated()
{
	if (item->outControl != nullptr &&
		(item->showOutControl != nullptr && item->showOutControl->boolValue()))
	{
		if (outControlUI == nullptr)
		{
			outControlUI.reset(new VolumeControlUI(item->outControl.get(), false));
			contentComponents.add(outControlUI.get());
			addAndMakeVisible(outControlUI.get());
		}
	}
	else if (outControlUI != nullptr)
	{
		removeChildComponent(outControlUI.get());
		contentComponents.removeAllInstancesOf(outControlUI.get());
		outControlUI.reset();
	}


	resized();
}

void BaseNodeViewUI::newMessage(const Node::NodeEvent& e)
{
	if (e.type == e.INPUTS_CHANGED || e.type == e.MIDI_INPUT_CHANGED) nodeInputsChanged();
	else if (e.type == e.OUTPUTS_CHANGED || e.type == e.MIDI_OUTPUT_CHANGED) nodeOutputsChanged();
	else if (e.type == e.VIEW_FILTER_UPDATED) viewFilterUpdated();
}


VolumeControlUI::VolumeControlUI(VolumeControl* item, bool showContour) :
	item(item),
	showContour(showContour)
{
	setViewedComponents(true, true, true);
}

void VolumeControlUI::setViewedComponents(bool showGain, bool showRMS, bool showActive)
{
	if (showGain)
	{
		if (gainUI == nullptr)
		{
			gainUI.reset(new DecibelSliderUI(item->gain));
			gainUI->customLabel = item->niceName;
			gainUI->orientation = FloatSliderUI::VERTICAL;
			addAndMakeVisible(gainUI.get());
		}
	}
	else
	{
		if (gainUI != nullptr)
		{
			removeChildComponent(gainUI.get());
			gainUI.reset();
		}
	}

	if (showRMS)
	{
		if (item->rms != nullptr)
		{
			if (rmsUI == nullptr)
			{
				rmsUI.reset(new RMSSliderUI(item->rms));
				rmsUI->orientation = FloatSliderUI::VERTICAL;

				addAndMakeVisible(rmsUI.get());
			}
		}
	}
	else
	{
		if (rmsUI != nullptr)
		{
			removeChildComponent(rmsUI.get());
			rmsUI.reset();
		}
	}

	if (showActive)
	{
		if (activeUI == nullptr)
		{
			activeUI.reset(item->active->createButtonToggle());
			activeUI->showLabel = false;
			addAndMakeVisible(activeUI.get());
		}
	}
	else
	{
		if (activeUI != nullptr)
		{
			removeChildComponent(activeUI.get());
			activeUI.reset();
		}
	}

	resized();
}

void VolumeControlUI::paint(Graphics& g)
{
	if (showContour)
	{
		g.setColour(NORMAL_COLOR);
		g.drawRoundedRectangle(getLocalBounds().reduced(1).toFloat(), 2, .5f);
	}
}

void VolumeControlUI::resized()
{
	Rectangle<int> r = getLocalBounds();
	if (showContour) r.reduce(2, 2);

	if(activeUI != nullptr) activeUI->setBounds(r.removeFromBottom(r.getWidth()).reduced(1));

	if (rmsUI != nullptr)
	{
		rmsUI->setBounds(r.removeFromRight(6));
		r.removeFromRight(1);
	}
	else
	{
		r.reduce(1, 0);
	}

	if(gainUI != nullptr) gainUI->setBounds(r);
}
