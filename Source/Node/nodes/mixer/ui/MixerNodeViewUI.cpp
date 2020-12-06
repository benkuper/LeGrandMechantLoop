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
    rebuildOutLine();
}

MixerNodeViewUI::~MixerNodeViewUI()
{
}

void MixerNodeViewUI::nodeInputsChanged()
{
    updateLines();
}

void MixerNodeViewUI::nodeOutputsChanged()
{
    for (auto& line : gainLines) line->rebuild();
    rebuildOutLine();
    resized();
}

void MixerNodeViewUI::updateLines()
{
    int numCC = node->gainRootCC.controllableContainers.size();
    while (gainLines.size() > numCC)
    {
        removeChildComponent(gainLines[gainLines.size() - 1]);
        gainLines.removeLast();
    }

    while (gainLines.size() < numCC)
    {
        InputGainLine* line = new InputGainLine(node->gainRootCC.controllableContainers[gainLines.size()], gainLines.size());
        addAndMakeVisible(line);
        gainLines.add(line);
    }

    resized();
}

void MixerNodeViewUI::rebuildOutLine()
{
    for (auto& ui : outGainsUI) removeChildComponent(ui);
    for (auto& ui : rmsUI) removeChildComponent(ui);
    outGainsUI.clear();
    rmsUI.clear();

    for (int i = 0; i < node->outGainsCC.controllables.size(); i++)
    {
        FloatSliderUI* gui = ((FloatParameter*)node->outGainsCC.controllables[i])->createSlider();
        gui->orientation = gui->VERTICAL;
        addAndMakeVisible(gui);
        outGainsUI.add(gui);

        FloatSliderUI* rui = (new RMSSliderUI((FloatParameter*)node->rmsCC.controllables[i]));
        addAndMakeVisible(rui);
        rmsUI.add(rui);
    }

}

void MixerNodeViewUI::paint(Graphics& g)
{
    BaseItemUI::paint(g);
    g.setColour(NORMAL_COLOR.withAlpha(.5f));
    g.drawRoundedRectangle(outRect.toFloat(), 2, 1);
}

void MixerNodeViewUI::resizedInternalContentNode(Rectangle<int>& r)
{
    if (outGainsUI.size() == 0) return;

    outRect = r.removeFromBottom(100).reduced(2);
    Rectangle<int> outR(outRect);
    
    int sizePerGain = jmin(outR.getWidth() / outGainsUI.size(), 30);
    for (int i = 0; i < outGainsUI.size(); i++)
    {
        Rectangle<int> gr = outR.removeFromLeft(sizePerGain).reduced(2);
        rmsUI[i]->setBounds(gr.removeFromRight(6));
        gr.removeFromRight(2);
        outGainsUI[i]->setBounds(gr);
    }

    r.removeFromTop(6);

    if (gainLines.size() == 0) return;

    int lineHeight = jmin(r.getHeight() / gainLines.size(),80);
    for (auto& line : gainLines) line->setBounds(r.removeFromTop(lineHeight).reduced(2));
}


// INPUT GAIN LINE

MixerNodeViewUI::InputGainLine::InputGainLine(ControllableContainer* gainsCC, int index) :
    index(index),
    gainsCC(gainsCC)
{
    rebuild();
}

void MixerNodeViewUI::InputGainLine::rebuild()
{
    for (auto& ui : gainsUI) removeChildComponent(ui);
    gainsUI.clear();

    for (auto& c : gainsCC->controllables)
    {
        FloatSliderUI * ui = ((FloatParameter*)c)->createSlider();
        ui->orientation = ui->VERTICAL;
        gainsUI.add(ui);
        addAndMakeVisible(ui);
    }

    resized();
}

void MixerNodeViewUI::InputGainLine::paint(Graphics& g)
{
    g.setColour(NORMAL_COLOR.withAlpha(.5f));
    g.drawRoundedRectangle(getLocalBounds().toFloat(), 2, 1);
}

void MixerNodeViewUI::InputGainLine::resized()
{
    if (gainsUI.size() == 0) return;

    Rectangle<int> r = getLocalBounds().reduced(2);
    int sizePerGain = jmin(r.getWidth() / gainsUI.size(), 30);
    for (int i = 0; i < gainsUI.size();i++)
    {
        Rectangle<int> gr = r.removeFromLeft(sizePerGain).reduced(2);
        gainsUI[i]->setBounds(gr.withSizeKeepingCentre(jmin(gr.getWidth(), 16),gr.getHeight()));
    }
}
