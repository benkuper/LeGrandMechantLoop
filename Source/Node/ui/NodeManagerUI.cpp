/*
  ==============================================================================

    NodeManagerUI.cpp
    Created: 15 Nov 2020 8:40:16am
    Author:  bkupe

  ==============================================================================
*/

#include "NodeManagerUI.h"

NodeManagerUI::NodeManagerUI(NodeManager * manager) :
    BaseManagerUI(manager->niceName, manager)
{
    addExistingItems();
}

NodeManagerUI::~NodeManagerUI()
{
}

NodeManagerPanel::NodeManagerPanel(StringRef contentName) :
    ShapeShifterContentComponent(contentName)
{
    setManager(RootNodeManager::getInstance());
}

NodeManagerPanel::~NodeManagerPanel()
{
}

void NodeManagerPanel::setManager(NodeManager* manager)
{
    if (managerUI != nullptr)
    {
        if (managerUI->manager == manager) return;
        removeChildComponent(managerUI.get());
    }

    if (manager != nullptr) managerUI.reset(new NodeManagerUI(manager));
    else managerUI.reset();

    if (managerUI != nullptr)
    {
        addAndMakeVisible(managerUI.get());
    }

    resized();
}

void NodeManagerPanel::resized()
{
    if (managerUI != nullptr) managerUI->setBounds(getLocalBounds());
}
