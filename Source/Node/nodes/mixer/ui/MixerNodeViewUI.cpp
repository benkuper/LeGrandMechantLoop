/*
  ==============================================================================

    MixerNodeViewUI.cpp
    Created: 15 Nov 2020 11:40:42pm
    Author:  bkupe

  ==============================================================================
*/

#include "MixerNodeViewUI.h"

MixerNodeViewUI::MixerNodeViewUI(MixerNode* n) :
    NodeViewUI(n)
{
    updateLines();
}

MixerNodeViewUI::~MixerNodeViewUI()
{
}

void MixerNodeViewUI::nodeInputsChanged()
{
    for (auto& line : gainLines) line->rebuild();
    resized(); 
}

void MixerNodeViewUI::nodeOutputsChanged()
{
    updateLines();
}

void MixerNodeViewUI::updateLines()
{
    int numLines = node->outputLines.size();
    while (gainLines.size() > numLines)
    {
        removeChildComponent(gainLines[gainLines.size() - 1]);
        gainLines.removeLast();
    }

    for (auto & l : gainLines) l->rebuild();

    while (gainLines.size() < numLines)
    {
        OutputGainLine* line = new OutputGainLine(node, node->outputLines[gainLines.size()]);
        addAndMakeVisible(line);
        gainLines.add(line);
    }


    resized();
}

void MixerNodeViewUI::viewFilterUpdated()
{
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
    int lineHeight = jmin(r.getHeight() / gainLines.size(), 140);
    for (auto& line : gainLines) line->setBounds(r.removeFromTop(lineHeight).reduced(2));
}


// INPUT GAIN LINE

MixerNodeViewUI::OutputGainLine::OutputGainLine(MixerNode * node, OutputLineCC * outputLine) :
    node(node),
    outputLine(outputLine)
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
            outUI.reset(new VolumeControlUI(&outputLine->out));
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

    if (node->showOutputExclusives->boolValue())
    {
        if (exclusiveUI == nullptr)
        {
            exclusiveUI.reset(outputLine->exclusiveMode->createButtonToggle());
            exclusiveUI->useCustomFGColor = true;
            exclusiveUI->customFGColor = Colours::purple.brighter(.2f);
            exclusiveUI->customLabel = "Ex.";
            addAndMakeVisible(exclusiveUI.get());
        }
    }
    else
    {
        if (exclusiveUI != nullptr)
        {
            removeChildComponent(exclusiveUI.get());
            exclusiveUI.reset();
        }
    }


    for (auto& ui : itemsUI) removeChildComponent(ui);
    itemsUI.clear();

    if (node->showItemGains->boolValue() || node->showItemActives->boolValue())
    {
        for (auto& mi : outputLine->mixerItems)
        {
            VolumeControlUI* mui = new VolumeControlUI(mi);
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
    if(outUI != nullptr || exclusiveUI != nullptr) r.removeFromRight(40).toFloat();

    g.drawRoundedRectangle(r, 2, 1);
}

void MixerNodeViewUI::OutputGainLine::resized()
{
    if (itemsUI.size() == 0) return;

    Rectangle<int> r = getLocalBounds().reduced(2);

    if (outUI != nullptr || exclusiveUI != nullptr)
    {
        Rectangle<int> rr = r.removeFromRight(30);
        
        if(exclusiveUI != nullptr) exclusiveUI->setBounds(rr.removeFromTop(rr.getWidth()).reduced(2));
        if(outUI != nullptr) outUI->setBounds(rr);
        
        r.removeFromRight(10);
    }
   
    int sizePerGain = jmin(r.getWidth() / itemsUI.size(), 30);
    for (int i = 0; i < itemsUI.size();i++)
    {
        itemsUI[i]->setBounds(r.removeFromLeft(sizePerGain).reduced(2));
    }
}
