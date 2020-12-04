/*
  ==============================================================================

    AudioRouterNodeUI.cpp
    Created: 4 Dec 2020 11:58:03am
    Author:  bkupe

  ==============================================================================
*/

#include "AudioRouterNodeUI.h"

AudioRouterNodeViewUI::AudioRouterNodeViewUI(GenericNode<AudioRouterProcessor>* n) :
    GenericNodeViewUI(n)
{
    currentInputUI.reset((ParameterEditor *)processor->currentInput->getEditor(false));
    addAndMakeVisible(currentInputUI.get());

    currentOutputUI.reset((ParameterEditor *)processor->currentOutput->getEditor(false));
    addAndMakeVisible(currentOutputUI.get());
}

AudioRouterNodeViewUI::~AudioRouterNodeViewUI()
{

}

void AudioRouterNodeViewUI::resizedInternalContentNode(Rectangle<int>& r)
{
    currentInputUI->setBounds(r.removeFromTop(20).reduced(2));
    currentOutputUI->setBounds(r.removeFromTop(20).reduced(2));

}
