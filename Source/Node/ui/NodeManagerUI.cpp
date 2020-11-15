/*
  ==============================================================================

    NodeManagerUI.cpp
    Created: 15 Nov 2020 8:40:16am
    Author:  bkupe

  ==============================================================================
*/

#include "NodeManagerUI.h"

NodeManagerUI::NodeManagerUI(StringRef name) :
    BaseManagerShapeShifterUI(name, RootNodeManager::getInstance())
{
    addExistingItems();

}

NodeManagerUI::~NodeManagerUI()
{
}
