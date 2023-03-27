/*
  ==============================================================================

	SamplerNodeUI.cpp
	Created: 14 Apr 2021 8:42:07am
	Author:  bkupe

  ==============================================================================
*/

#include "Node/NodeIncludes.h"

SamplerNodeViewUI::SamplerNodeViewUI(SamplerNode* n) :
	NodeViewUI(n),
	midiComp(n, n->keyboardState, MidiKeyboardComponent::horizontalKeyboard)
{
	midiParamUI.reset(node->midiInterfaceParam->createTargetUI());
	addAndMakeVisible(midiParamUI.get());

	addAndMakeVisible(&midiComp);
	midiComp.addChangeListener(this);

	midiComp.setLowestVisibleKey(node->viewStartKey);

	contentComponents.add(midiParamUI.get());
	contentComponents.add(&midiComp);
}

SamplerNodeViewUI::~SamplerNodeViewUI()
{
}

void SamplerNodeViewUI::resizedInternalHeader(Rectangle<int>& r)
{
	NodeViewUI::resizedInternalHeader(r);
}

void SamplerNodeViewUI::resizedInternalContentNode(Rectangle<int>& r)
{
	midiParamUI->setBounds(r.removeFromTop(20).removeFromRight(120).reduced(1));
	r.removeFromTop(8);
	midiComp.setBounds(r.reduced(1));
}

void SamplerNodeViewUI::controllableFeedbackUpdateInternal(Controllable* c)
{
	NodeViewUI::controllableFeedbackUpdateInternal(c);
	if (c->parentContainer == &node->noteStatesCC) midiComp.repaint();
}

void SamplerNodeViewUI::changeListenerCallback(ChangeBroadcaster* source)
{
	if (source == &midiComp)
	{
		node->viewStartKey = midiComp.getLowestVisibleKey();
	}
}


SamplerMidiKeyboardComponent::SamplerMidiKeyboardComponent(SamplerNode* n, MidiKeyboardState& state, MidiKeyboardComponent::Orientation orientation) :
	samplerNode(n),
	MidiKeyboardComponent(state, orientation)
{
}

void SamplerMidiKeyboardComponent::drawWhiteNote(int midiNoteNumber, Graphics& g, Rectangle<float> area,
	bool isDown, bool isOver, Colour lineColour, Colour textColour)
{
	auto c = samplerNode->samplerNotes[midiNoteNumber]->hasContent() ? BLUE_COLOR.brighter() : Colours::transparentWhite;

	SamplerNode::NoteState s = samplerNode->samplerNotes[midiNoteNumber]->state->getValueDataAsEnum<SamplerNode::NoteState>();
	if (isDown) c = s == SamplerNode::RECORDING ? RED_COLOR : GREEN_COLOR;
	if (isOver)  c = c.overlaidWith(findColour(mouseOverKeyOverlayColourId));

	g.setColour(c);
	g.fillRect(area);

	auto text = getWhiteNoteText(midiNoteNumber);

	if (text.isNotEmpty())
	{
		auto fontHeight = jmin(12.0f, getKeyWidth() * 0.9f);

		g.setColour(textColour);
		g.setFont(Font(fontHeight).withHorizontalScale(0.8f));

		switch (getOrientation())
		{
		case horizontalKeyboard:            g.drawText(text, area.withTrimmedLeft(1.0f).withTrimmedBottom(2.0f), Justification::centredBottom, false); break;
		case verticalKeyboardFacingLeft:    g.drawText(text, area.reduced(2.0f), Justification::centredLeft, false); break;
		case verticalKeyboardFacingRight:   g.drawText(text, area.reduced(2.0f), Justification::centredRight, false); break;
		default: break;
		}
	}

	if (!lineColour.isTransparent())
	{
		g.setColour(lineColour);

		switch (getOrientation())
		{
		case horizontalKeyboard:            g.fillRect(area.withWidth(1.0f)); break;
		case verticalKeyboardFacingLeft:    g.fillRect(area.withHeight(1.0f)); break;
		case verticalKeyboardFacingRight:   g.fillRect(area.removeFromBottom(1.0f)); break;
		default: break;
		}

		if (midiNoteNumber == getRangeEnd())
		{
			switch (getOrientation())
			{
			case horizontalKeyboard:            g.fillRect(area.expanded(1.0f, 0).removeFromRight(1.0f)); break;
			case verticalKeyboardFacingLeft:    g.fillRect(area.expanded(0, 1.0f).removeFromBottom(1.0f)); break;
			case verticalKeyboardFacingRight:   g.fillRect(area.expanded(0, 1.0f).removeFromTop(1.0f)); break;
			default: break;
			}
		}
	}
}

void SamplerMidiKeyboardComponent::drawBlackNote(int midiNoteNumber, Graphics& g, Rectangle<float> area,
	bool isDown, bool isOver, Colour noteFillColour)
{
	auto c = samplerNode->samplerNotes[midiNoteNumber]->hasContent() ? BLUE_COLOR.darker() : noteFillColour;

	SamplerNode::NoteState s = samplerNode->samplerNotes[midiNoteNumber]->state->getValueDataAsEnum<SamplerNode::NoteState>();
	if (isDown) c = s == SamplerNode::RECORDING ? RED_COLOR : GREEN_COLOR;
	if (isOver)  c = c.overlaidWith(findColour(mouseOverKeyOverlayColourId));

	g.setColour(c);
	g.fillRect(area);

	if (isDown)
	{
		g.setColour(noteFillColour);
		g.drawRect(area);
	}
	else
	{
		g.setColour(c.brighter());
		auto sideIndent = 1.0f / 8.0f;
		auto topIndent = 7.0f / 8.0f;
		auto w = area.getWidth();
		auto h = area.getHeight();

		switch (getOrientation())
		{
		case horizontalKeyboard:            g.fillRect(area.reduced(w * sideIndent, 0).removeFromTop(h * topIndent)); break;
		case verticalKeyboardFacingLeft:    g.fillRect(area.reduced(0, h * sideIndent).removeFromRight(w * topIndent)); break;
		case verticalKeyboardFacingRight:   g.fillRect(area.reduced(0, h * sideIndent).removeFromLeft(w * topIndent)); break;
		default: break;
		}
	}
}
