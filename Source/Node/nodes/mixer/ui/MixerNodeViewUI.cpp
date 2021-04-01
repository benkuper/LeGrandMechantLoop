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

    while (gainLines.size() < numLines)
    {
        OutputGainLine* line = new OutputGainLine(node->outputLines[gainLines.size()]);
        addAndMakeVisible(line);
        gainLines.add(line);
    }

    resized();
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

MixerNodeViewUI::OutputGainLine::OutputGainLine(OutputLineCC * outputLine) :
    outputLine(outputLine),
    outUI(&outputLine->out)
{
    exclusiveUI.reset(outputLine->exclusiveMode->createButtonToggle());
    addAndMakeVisible(exclusiveUI.get());
    exclusiveUI->useCustomFGColor = true;
    exclusiveUI->customFGColor = Colours::purple.brighter(.2f);
    exclusiveUI->customLabel = "Ex.";
    addAndMakeVisible(&outUI);
    rebuild();
}

void MixerNodeViewUI::OutputGainLine::rebuild()
{
    for (auto& ui : itemsUI) removeChildComponent(ui);
    itemsUI.clear();

    for (auto& mi : outputLine->mixerItems)
    {
        VolumeControlUI  * mui = new VolumeControlUI(mi);
        itemsUI.add(mui);
        addAndMakeVisible(mui);
    }

    resized();
}

void MixerNodeViewUI::OutputGainLine::paint(Graphics& g)
{
    g.setColour(NORMAL_COLOR.withAlpha(.5f));
    Rectangle<float> r = getLocalBounds().toFloat();
    r.removeFromRight(40).toFloat();

    g.drawRoundedRectangle(r, 2, 1);
}

void MixerNodeViewUI::OutputGainLine::resized()
{
    if (itemsUI.size() == 0) return;

    Rectangle<int> r = getLocalBounds().reduced(2);

    Rectangle<int> rr = r.removeFromRight(30);
    exclusiveUI->setBounds(rr.removeFromTop(rr.getWidth()).reduced(2));
    outUI.setBounds(rr);
    r.removeFromRight(10);

    int sizePerGain = jmin(r.getWidth() / itemsUI.size(), 30);
    for (int i = 0; i < itemsUI.size();i++)
    {
        itemsUI[i]->setBounds(r.removeFromLeft(sizePerGain).reduced(2));
    }
}
