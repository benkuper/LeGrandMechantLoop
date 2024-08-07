/*
  ==============================================================================

    IONodeViewUI.cpp
    Created: 15 Nov 2020 11:39:57pm
    Author:  bkupe

  ==============================================================================
*/

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
    int realNumChannels = node->isInput ? node->realNumOutputs : node->realNumInputs;

    for (int i = 0; i < numChannels; i++)
    {
        VolumeControlUI* channelUI = new VolumeControlUI((VolumeControl*)node->channelsCC.controllableContainers[i].get(), true, String(i+1));
        if (i >= realNumChannels)
        {
            channelUI->gainUI->useCustomFGColor = true;
            channelUI->gainUI->customFGColor = Colours::grey;
        }
        contentComponents.add(channelUI);
        channelsUI.add(channelUI);
        addAndMakeVisible(channelUI);
    }

    resized();
}

void IONodeViewUI::resizedInternalContentNode(Rectangle<int>& r)
{
    for (auto& ui : channelsUI) ui->setBounds(r.removeFromLeft(40).reduced(6));
}
