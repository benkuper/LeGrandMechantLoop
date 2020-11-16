/*
  ==============================================================================

    NodeManagerViewUI.cpp
    Created: 15 Nov 2020 8:40:22am
    Author:  bkupe

  ==============================================================================
*/

#include "NodeManagerViewUI.h"
#include "../Connection/ui/NodeConnectionManagerViewUI.h"

NodeManagerViewUI::NodeManagerViewUI(StringRef name) :
    BaseManagerShapeShifterViewUI(name, RootNodeManager::getInstance())
{
    connectionManagerUI.reset(new NodeConnectionManagerViewUI(this, manager->connectionManager.get()));
    addAndMakeVisible(connectionManagerUI.get());

    enableSnapping = true;

    updatePositionOnDragMove = true;
    addExistingItems();
}

NodeManagerViewUI::~NodeManagerViewUI()
{
}

NodeViewUI* NodeManagerViewUI::createUIForItem(Node* n)
{
    return n->createViewUI();
}

void NodeManagerViewUI::resizedInternalContent(Rectangle<int> &r)
{
    BaseManagerViewUI::resizedInternalContent(r);
    connectionManagerUI->setBounds(r);
}
