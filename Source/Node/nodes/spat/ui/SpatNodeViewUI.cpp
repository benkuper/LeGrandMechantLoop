/*
  ==============================================================================

    SpatNodeViewUI.cpp
    Created: 15 Nov 2020 11:41:00pm
    Author:  bkupe

  ==============================================================================
*/

#include "SpatNodeViewUI.h"

SpatNodeViewUI::SpatNodeViewUI(GenericAudioNode<SpatProcessor>* n) :
    GenericAudioNodeViewUI(n)
{
    removeMouseListener(this);
    addMouseListener(this, true);
    setDisableDefaultMouseEvents(true);

    updateItemsUI();
}

SpatNodeViewUI::~SpatNodeViewUI()
{
}

void SpatNodeViewUI::updateItemsUI()
{
    while (itemsUI.size() > audioNode->numOutputs)
    {
        removeChildComponent(itemsUI[itemsUI.size()]);
        itemsUI.removeLast();
    }

    while (itemsUI.size() < audioNode->numOutputs)
    {
        SpatItemUI* si = new SpatItemUI((SpatItem*)audioNode->processor->spatCC.controllableContainers[itemsUI.size()].get());
        addAndMakeVisible(si);
        itemsUI.add(si);
    }

    resized();
}

void SpatNodeViewUI::nodeOutputsChanged()
{
    updateItemsUI();
}

void SpatNodeViewUI::paint(Graphics& g)
{
    g.setColour(BG_COLOR.darker(.1f));
    g.fillRoundedRectangle(spatRect.toFloat(), 2);
    g.setColour(BG_COLOR.brighter(.2f));
    g.drawRoundedRectangle(spatRect.toFloat(), 2, 1);

    g.setColour(AUDIO_COLOR);
    g.fillEllipse(posRect.toFloat());
}

void SpatNodeViewUI::resizedInternalContentNode(Rectangle<int> &r)
{
    float minSize = jmin(r.getWidth(), r.getHeight());
    spatRect = r.withSizeKeepingCentre(minSize, minSize).reduced(4);

    Point<int> centre = spatRect.getRelativePoint(audioNode->processor->spatPosition->x, audioNode->processor->spatPosition->y);
    posRect = Rectangle<int>(30, 30).withCentre(centre);
    
    for (auto& s : itemsUI)
    {
        placeItem(s);
    }
}

void SpatNodeViewUI::placeItem(SpatItemUI* ui)
{
    if (ui == nullptr) return;

    float size = ui->item->radius->floatValue() * 2;
    Rectangle<int> bounds = spatRect.getProportion(Rectangle<float>().withSize(size, size).withCentre(ui->item->position->getPoint()));
    ui->setBounds(bounds);
}

SpatItemUI* SpatNodeViewUI::getUIForItem(SpatItem* item)
{
    for (auto& ui : itemsUI) if (ui->item == item) return ui;
    return nullptr;
}

void SpatNodeViewUI::mouseDrag(const MouseEvent& e)
{
    if (audioNode->processor->spatMode->getValueDataAsEnum<SpatProcessor::SpatMode>() != SpatProcessor::FREE) return;

    if (SpatItemUI* si = dynamic_cast<SpatItemUI*>(e.eventComponent))
    {
        Point<float> siPos = (si->getBounds().getCentre() - spatRect.getTopLeft()).toFloat() / Point<float>(spatRect.getWidth(), spatRect.getHeight());
        si->item->position->setPoint(siPos);
    }
}

void SpatNodeViewUI::mouseUp(const MouseEvent& e)
{
    if (audioNode->processor->spatMode->getValueDataAsEnum<SpatProcessor::SpatMode>() != SpatProcessor::FREE) return;

    if (SpatItemUI* si = dynamic_cast<SpatItemUI*>(e.eventComponent))
    {
        if (si->posAtMouseDown != si->item->position->getPoint())
        {
            si->item->position->setUndoablePoint(si->posAtMouseDown, si->item->position->getPoint());
        }
    }
}

void SpatNodeViewUI::controllableFeedbackUpdateInternal(Controllable* c)
{
    GenericAudioNodeViewUI::controllableFeedbackUpdateInternal(c);
    if (c == audioNode->processor->spatPosition || c == audioNode->processor->circleRadius || c == audioNode->processor->circleAngle || c == audioNode->processor->spatRadius)
    {
        resized();
        repaint();
    }
    else if (SpatItem* si = c->getParentAs<SpatItem>())
    {
        placeItem(getUIForItem(si));
    }
}

