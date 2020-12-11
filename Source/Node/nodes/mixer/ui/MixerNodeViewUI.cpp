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
    int numCC = node->itemsCC.controllableContainers.size();
    while (gainLines.size() > numCC)
    {
        removeChildComponent(gainLines[gainLines.size() - 1]);
        gainLines.removeLast();
    }

    while (gainLines.size() < numCC)
    {
        InputGainLine* line = new InputGainLine(node->itemsCC.controllableContainers[gainLines.size()], gainLines.size());
        addAndMakeVisible(line);
        gainLines.add(line);
    }

    resized();
}

void MixerNodeViewUI::rebuildOutLine()
{
    for (auto& ui : outItemsUI) removeChildComponent(ui);
    outItemsUI.clear();

    for (int i = 0; i < node->outItemsCC.controllableContainers.size(); i++)
    {
        MixerItemUI* mi = new MixerItemUI((MixerItem*)node->outItemsCC.controllableContainers[i].get());
        addAndMakeVisible(mi);
        outItemsUI.add(mi);
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
    if (outItemsUI.size() == 0) return;

    outRect = r.removeFromBottom(100).reduced(2);
    Rectangle<int> outR(outRect);
    
    int sizePerGain = jmin(outR.getWidth() / outItemsUI.size(), 30);
    for (int i = 0; i < outItemsUI.size(); i++)
    {
        outItemsUI[i]->setBounds(outR.removeFromLeft(sizePerGain).reduced(2));
    }

    r.removeFromTop(6);

    if (gainLines.size() == 0) return;

    int lineHeight = jmin(r.getHeight() / gainLines.size(),120);
    for (auto& line : gainLines) line->setBounds(r.removeFromTop(lineHeight).reduced(2));
}


// INPUT GAIN LINE

MixerNodeViewUI::InputGainLine::InputGainLine(ControllableContainer* itemsCC, int index) :
    index(index),
    itemsCC(itemsCC)
{
    rebuild();
}

void MixerNodeViewUI::InputGainLine::rebuild()
{
    for (auto& ui : itemsUI) removeChildComponent(ui);
    itemsUI.clear();

    for (auto& cc : itemsCC->controllableContainers)
    {
        MixerItemUI * mi = new MixerItemUI((MixerItem *)cc.get());
        itemsUI.add(mi);
        addAndMakeVisible(mi);
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
    if (itemsUI.size() == 0) return;

    Rectangle<int> r = getLocalBounds().reduced(2);
    int sizePerGain = jmin(r.getWidth() / itemsUI.size(), 30);
    for (int i = 0; i < itemsUI.size();i++)
    {
        itemsUI[i]->setBounds(r.removeFromLeft(sizePerGain).reduced(2));
    }
}



MixerItemUI::MixerItemUI(MixerItem* item) :
    item(item)
{
    gainUI.reset(item->gain->createSlider());
    gainUI->orientation = gainUI->VERTICAL;
    gainUI->customLabel = item->niceName;
    addAndMakeVisible(gainUI.get());
    if (item->rms != nullptr)
    {
        rmsUI.reset(new RMSSliderUI(item->rms));
        addAndMakeVisible(rmsUI.get());
    }
   
    activeUI.reset(item->active->createButtonToggle());
    activeUI->showLabel = false;
    addAndMakeVisible(activeUI.get());
}

void MixerItemUI::paint(Graphics& g)
{
    g.setColour(NORMAL_COLOR);
    g.drawRoundedRectangle(getLocalBounds().reduced(1).toFloat(), 2, 1);
}

void MixerItemUI::resized()
{
    Rectangle<int> r = getLocalBounds().reduced(2);
    activeUI->setBounds(r.removeFromBottom(r.getWidth()).reduced(2));

    if (rmsUI != nullptr)
    {
        rmsUI->setBounds(r.removeFromRight(8));
        r.removeFromRight(2);
    }
    else
    {
        r.reduce(2, 0);
    }
    
    gainUI->setBounds(r);
}
