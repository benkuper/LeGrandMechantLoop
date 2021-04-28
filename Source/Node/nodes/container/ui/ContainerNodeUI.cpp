/*
  ==============================================================================

    ContainerNodeUI.cpp
    Created: 3 Dec 2020 9:43:14am
    Author:  bkupe

  ==============================================================================
*/

ContainerNodeViewUI::ContainerNodeViewUI(ContainerNode* node) :
    NodeViewUI(node)
{
    editHeaderBT.reset(AssetManager::getInstance()->getEditBT());
    addAndMakeVisible(editHeaderBT.get());
    editHeaderBT->addListener(this);
}

ContainerNodeViewUI::~ContainerNodeViewUI()
{
}

void ContainerNodeViewUI::resizedInternalHeader(Rectangle<int>& r)
{
    NodeViewUI::resizedInternalHeader(r);
    editHeaderBT->setBounds(r.removeFromRight(r.getHeight()).reduced(1));
}


void ContainerNodeViewUI::buttonClicked(Button* b)
{
    NodeViewUI::buttonClicked(b);
    if (b == editHeaderBT.get())
    {
        if (NodeManagerViewPanel* mui = ShapeShifterManager::getInstance()->getContentForType<NodeManagerViewPanel>())
        {
            mui->setManager(node->nodeManager.get());
        }
    }
}
