/*
  ==============================================================================

    NodeViewUI.cpp
    Created: 15 Nov 2020 9:26:57am
    Author:  bkupe

  ==============================================================================
*/

#include "NodeViewUI.h"

NodeViewUI::NodeViewUI(Node* node) :
    BaseItemUI(node, Direction::ALL)
{
    dragAndDropEnabled = true;
    autoHideWhenDragging = false;
    drawEmptyDragIcon = true;

    setSize(300, 200);
}

NodeViewUI::~NodeViewUI()
{
}
