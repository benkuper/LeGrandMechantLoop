/*
  ==============================================================================

    ContainerNodeUI.cpp
    Created: 3 Dec 2020 9:43:14am
    Author:  bkupe

  ==============================================================================
*/

#include "ContainerNodeUI.h"
#include "Node/ui/NodeManagerViewUI.h"

ContainerNodeViewUI::ContainerNodeViewUI(ContainerNode* node) :
    NodeViewUI(node),
    editBT("Edit")
{
    editHeaderBT.reset(AssetManager::getInstance()->getEditBT());
    addAndMakeVisible(editHeaderBT.get());
    editHeaderBT->addListener(this);

    editBT.addListener(this);
    addAndMakeVisible(&editBT);

    contentComponents.add(&editBT);
}

ContainerNodeViewUI::~ContainerNodeViewUI()
{
}

void ContainerNodeViewUI::resizedInternalHeader(Rectangle<int>& r)
{
    NodeViewUI::resizedInternalHeader(r);
    editHeaderBT->setBounds(r.removeFromRight(r.getHeight()).reduced(1));
}

void ContainerNodeViewUI::resizedInternalContentNode(Rectangle<int>& r)
{
    editBT.setBounds(r.removeFromTop(40).reduced(4));
}

void ContainerNodeViewUI::buttonClicked(Button* b)
{
    NodeViewUI::buttonClicked(b);
    if (b == &editBT || b == editHeaderBT.get())
    {
        if (NodeManagerViewPanel* mui = ShapeShifterManager::getInstance()->getContentForType<NodeManagerViewPanel>())
        {
            mui->setManager(node->nodeManager.get());
        }
    }
}
