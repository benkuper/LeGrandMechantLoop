/*
  ==============================================================================

    NodeConnectionViewUI.h
    Created: 16 Nov 2020 10:01:02am
    Author:  bkupe

  ==============================================================================
*/

#pragma once
#include "../NodeConnection.h"

class NodeViewUI;

class NodeConnectionViewUI :
    public BaseItemMinimalUI<NodeConnection>
{
public:
    NodeConnectionViewUI(NodeConnection* connection, NodeViewUI * sourceUI = nullptr, NodeViewUI * destUI = nullptr);
    ~NodeConnectionViewUI();

    NodeViewUI* sourceUI;
    NodeViewUI* destUI;

    void updateBounds();
};