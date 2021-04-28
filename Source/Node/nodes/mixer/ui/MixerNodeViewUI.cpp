/*
  ==============================================================================

    MixerNodeViewUI.cpp
    Created: 15 Nov 2020 11:40:42pm
    Author:  bkupe

  ==============================================================================
*/

MixerNodeViewUI::MixerNodeViewUI(MixerNode* n) :
    NodeViewUI(n)
{
    updateExclusives();
    updateLines();
}

MixerNodeViewUI::~MixerNodeViewUI()
{
}

void MixerNodeViewUI::nodeInputsChanged()
{
    for (auto& line : gainLines) line->rebuild();
    updateExclusives();
    resized(); 
}

void MixerNodeViewUI::nodeOutputsChanged()
{
    updateLines();
}

void MixerNodeViewUI::updateLines()
{
    int numLines = node->getNumAudioOutputs();
    while (gainLines.size() > numLines)
    {
        Component* c = gainLines[gainLines.size() - 1];
        removeChildComponent(c);
        contentComponents.removeAllInstancesOf(c);
        gainLines.removeLast();
    }

    for (auto & l : gainLines) l->rebuild();

    while (gainLines.size() < numLines)
    {
        OutputGainLine* line = new OutputGainLine(node, gainLines.size());
        contentComponents.add(line);
        addAndMakeVisible(line);
        gainLines.add(line);
    }


    resized();
}

void MixerNodeViewUI::updateExclusives()
{
    if (node->getNumAudioInputs() == 0) return;

    if (node->showOutputExclusives->boolValue())
    {
        while (exclusivesUI.size() > node->inputLines.size())
        {
            IntParameterLabelUI * eui = exclusivesUI[exclusivesUI.size() - 1];
            contentComponents.removeAllInstancesOf(eui);
            removeChildComponent(eui);
            exclusivesUI.removeObject(eui);
        }

        while (exclusivesUI.size() < node->inputLines.size())
        {
            IntParameterLabelUI* eui = node->inputLines[exclusivesUI.size()]->exclusiveIndex->createLabelUI();
            exclusivesUI.add(eui);
            contentComponents.add(eui);
            addAndMakeVisible(eui);
        }
    }
    else
    {
        for (auto& eui : exclusivesUI)
        {
            removeChildComponent(eui);
            contentComponents.removeAllInstancesOf(eui);
        }
        exclusivesUI.clear();
    }

}

void MixerNodeViewUI::viewFilterUpdated()
{
    updateExclusives();
    updateLines();
    NodeViewUI::viewFilterUpdated();
}

void MixerNodeViewUI::paint(Graphics& g)
{
    BaseItemUI::paint(g);
    g.setColour(NORMAL_COLOR.withAlpha(.5f));
}

void MixerNodeViewUI::resizedInternalContentNode(Rectangle<int>& r)
{
    if (gainLines.size() == 0) return;
    int lineHeight = jmin((r.getHeight() - 16) / gainLines.size(), 140);

    if (exclusivesUI.size() > 0)
    {
        Rectangle<int> er = r.removeFromBottom(16).reduced(1);
        int sizePerGain = jmin(er.getWidth() / exclusivesUI.size(), 30);

        for (int i = 0; i < exclusivesUI.size(); i++)
        {
            exclusivesUI[i]->setBounds(er.removeFromLeft(sizePerGain).reduced(1));
        }
    }

    for (auto& line : gainLines) line->setBounds(r.removeFromTop(lineHeight).reduced(1));

}


// INPUT GAIN LINE

MixerNodeViewUI::OutputGainLine::OutputGainLine(MixerNode * node, int outputIndex) :
    node(node),
    outputIndex(outputIndex)
{
    rebuild();
}

void MixerNodeViewUI::OutputGainLine::rebuild()
{
    if (node->showOutputActives->boolValue() ||
        node->showOutputGains->boolValue() ||
        node->showOutputRMS->boolValue()
        )
    {
        if (outUI == nullptr)
        {
            outUI.reset(new VolumeControlUI(node->mainOuts[outputIndex]));
            addAndMakeVisible(outUI.get());
        }
        
        outUI->setViewedComponents(node->showOutputGains->boolValue(), node->showOutputRMS->boolValue(), node->showOutputActives->boolValue());

    }
    else
    {
        if (outUI != nullptr)
        {
            removeChildComponent(outUI.get());
            outUI.reset();
        }
    }

    for (auto& ui : itemsUI) removeChildComponent(ui);
    itemsUI.clear();

    if (node->showItemGains->boolValue() || node->showItemActives->boolValue())
    {
        for (int i = 0; i < node->getNumAudioInputs(); i++)
        {
            VolumeControlUI* mui = new VolumeControlUI(node->getMixerItem(i, outputIndex));
            mui->setViewedComponents(node->showItemGains->boolValue(), false, node->showItemActives->boolValue());
            itemsUI.add(mui);
            addAndMakeVisible(mui);
        }
    }

    resized();
}

void MixerNodeViewUI::OutputGainLine::paint(Graphics& g)
{
    g.setColour(NORMAL_COLOR.withAlpha(.5f));
    Rectangle<float> r = getLocalBounds().toFloat();
    if(outUI != nullptr) r.removeFromRight(40).toFloat();

    g.drawRoundedRectangle(r, 2, .5);
}

void MixerNodeViewUI::OutputGainLine::resized()
{
    if (itemsUI.size() == 0) return;

    Rectangle<int> r = getLocalBounds().reduced(1);

    if (outUI != nullptr )
    {
        Rectangle<int> rr = r.removeFromRight(30);
        outUI->setBounds(rr);
        r.removeFromRight(10);
    }
   
    int sizePerGain = jmin(r.getWidth() / itemsUI.size(), 30);
    for (int i = 0; i < itemsUI.size();i++)
    {
        itemsUI[i]->setBounds(r.removeFromLeft(sizePerGain).reduced(2));
    }
}
