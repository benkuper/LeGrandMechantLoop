/*
  ==============================================================================

    IONodeViewUI.cpp
    Created: 15 Nov 2020 11:39:57pm
    Author:  bkupe

  ==============================================================================
*/

#include "IONodeViewUI.h"

IONodeViewUI::IONodeViewUI(IONode * n) :
    NodeViewUI(n)
{
    updateUI();
}

IONodeViewUI::~IONodeViewUI()
{
}

void IONodeViewUI::nodeInputsChanged()
{
    NodeViewUI::nodeInputsChanged();
    if (!node->isInput) updateUI();
}

void IONodeViewUI::nodeOutputsChanged()
{
    NodeViewUI::nodeOutputsChanged();
    if(node->isInput) updateUI();
}

void IONodeViewUI::updateUI()
{
    for (auto& i : channelsUI)
    {
        contentComponents.removeAllInstancesOf(i);
        removeChildComponent(i);
    }
    channelsUI.clear();

    int numChannels = node->isInput ? node->getNumAudioOutputs() : node->getNumAudioInputs();

    for (int i = 0; i < numChannels; i++)
    {
        VolumeControlUI* channelUI = new VolumeControlUI((VolumeControl*)node->channelsCC.controllableContainers[i].get());
        contentComponents.add(channelUI);
        channelsUI.add(channelUI);
        addAndMakeVisible(channelUI);
    }

    resized();
}

void IONodeViewUI::resizedInternalContentNode(Rectangle<int>& r)
{
    for (auto& ui : channelsUI) ui->setBounds(r.removeFromLeft(45).reduced(6));
}
