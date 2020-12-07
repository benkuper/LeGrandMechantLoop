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
        IOChannelUI* channelUI = new IOChannelUI((IOChannel*)node->channelsCC.controllableContainers[i].get());
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

IOChannelUI::IOChannelUI(IOChannel* ioChannel) :
    ioChannel(ioChannel)
{
    gainUI.reset(ioChannel->gain->createSlider());
    gainUI->orientation = gainUI->VERTICAL;
    gainUI->customLabel = ioChannel->niceName;
    addAndMakeVisible(gainUI.get());
    rmsUI.reset(new RMSSliderUI(ioChannel->rms));
    addAndMakeVisible(rmsUI.get());
    activeUI.reset(ioChannel->active->createButtonToggle());
    addAndMakeVisible(activeUI.get());
}

void IOChannelUI::paint(Graphics& g)
{
    g.setColour(NORMAL_COLOR);
    g.drawRoundedRectangle(getLocalBounds().reduced(1).toFloat(), 2, 1);
}

void IOChannelUI::resized()
{
    Rectangle<int> r = getLocalBounds().reduced(2);
    activeUI->setBounds(r.removeFromBottom(r.getWidth()).reduced(2));

    rmsUI->setBounds(r.removeFromRight(8));
    r.removeFromRight(2);

    gainUI->setBounds(r);
}
