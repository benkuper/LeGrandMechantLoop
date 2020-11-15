/*
  ==============================================================================

    NodeManagerViewUI.cpp
    Created: 15 Nov 2020 8:40:22am
    Author:  bkupe

  ==============================================================================
*/

#include "NodeManagerViewUI.h"

NodeManagerViewUI::NodeManagerViewUI(StringRef name) :
    BaseManagerShapeShifterViewUI(name, RootNodeManager::getInstance())
{
    updatePositionOnDragMove = true;
    addExistingItems();
}

NodeManagerViewUI::~NodeManagerViewUI()
{
}
