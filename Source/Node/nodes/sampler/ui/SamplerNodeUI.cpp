/*
  ==============================================================================

    SamplerNodeUI.cpp
    Created: 14 Apr 2021 8:42:07am
    Author:  bkupe

  ==============================================================================
*/

#include "SamplerNodeUI.h"
#include "Common/MIDI/ui/MIDIDeviceParameterUI.h"

SamplerNodeViewUI::SamplerNodeViewUI(SamplerNode* n) :
    NodeViewUI(n),
    midiComp(n->keyboardState, MidiKeyboardComponent::horizontalKeyboard)
{
    midiParamUI.reset(node->midiParam->createMIDIParameterUI());
    addAndMakeVisible(midiParamUI.get());

    addAndMakeVisible(&midiComp);

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
}