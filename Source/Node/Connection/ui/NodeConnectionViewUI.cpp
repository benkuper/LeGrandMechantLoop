/*
  ==============================================================================

    NodeConnectionViewUI.cpp
    Created: 16 Nov 2020 10:01:02am
    Author:  bkupe

  ==============================================================================
*/

#include "NodeConnectionViewUI.h"

NodeConnectionViewUI::NodeConnectionViewUI(NodeConnection* connection, NodeViewUI* sourceUI, NodeViewUI* destUI) :
    BaseItemMinimalUI(connection),
    sourceUI(sourceUI),
    destUI(destUI)
{
}

NodeConnectionViewUI::~NodeConnectionViewUI()
{
}

void NodeConnectionViewUI::updateBounds()
{
}
