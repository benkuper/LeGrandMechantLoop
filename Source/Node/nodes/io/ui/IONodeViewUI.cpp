/*
  ==============================================================================

    IONodeViewUI.cpp
    Created: 15 Nov 2020 11:39:57pm
    Author:  bkupe

  ==============================================================================
*/

#include "IONodeViewUI.h"

InputNodeViewUI::InputNodeViewUI(GenericAudioNode<AudioInputProcessor>* n) :
    GenericAudioNodeViewUI(n)
{
    updateRMSUI();
}

InputNodeViewUI::~InputNodeViewUI()
{
}

void InputNodeViewUI::nodeOutputsChanged()
{
    NodeViewUI::nodeOutputsChanged();
    updateRMSUI();
}

void InputNodeViewUI::updateRMSUI()
{
    while (inputRMSUI.size() > item->audioOutputNames.size())
    {
        FloatSliderUI* s = inputRMSUI[inputRMSUI.size() - 1];
        removeChildComponent(s);
        inputRMSUI.removeObject(s);
    }

    while (inputRMSUI.size() < item->audioOutputNames.size())
    {
        FloatSliderUI* s = processor->inputRMS[inputRMSUI.size()]->createSlider();
        s->orientation = s->VERTICAL;
        s->showLabel = false;
        addAndMakeVisible(s);
        inputRMSUI.add(s);
    }

    resized();
}

void InputNodeViewUI::resizedInternalContent(Rectangle<int>& r)
{
    for (auto& rmsUI : inputRMSUI)
    {
        rmsUI->setBounds(r.removeFromLeft(20).reduced(2));
    }
}

OutputNodeViewUI::OutputNodeViewUI(GenericAudioNode<AudioOutputProcessor>* n) :
    GenericAudioNodeViewUI(n)
{
}

OutputNodeViewUI::~OutputNodeViewUI()
{
}
