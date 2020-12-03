/*
  ==============================================================================

    ContainerNodeUI.cpp
    Created: 3 Dec 2020 9:43:14am
    Author:  bkupe

  ==============================================================================
*/

#include "ContainerNodeUI.h"
#include "Node/ui/NodeManagerViewUI.h"

ContainerNodeViewUI::ContainerNodeViewUI(GenericNode<ContainerProcessor> * node) :
    GenericNodeViewUI(node),
    editBT("Edit")
{
    editBT.addListener(this);
    addAndMakeVisible(&editBT);
}

ContainerNodeViewUI::~ContainerNodeViewUI()
{
}

void ContainerNodeViewUI::resizedInternalContentNode(Rectangle<int>& r)
{
    editBT.setBounds(r.withSizeKeepingCentre(100, 20));
}

void ContainerNodeViewUI::buttonClicked(Button* b)
{
    GenericNodeViewUI::buttonClicked(b);
    if (b == &editBT)
    {
        if (NodeManagerViewPanel* mui = ShapeShifterManager::getInstance()->getContentForType<NodeManagerViewPanel>())
        {
            mui->setManager(processor->nodeManager.get());
        }
    }
}
