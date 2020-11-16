/*
  ==============================================================================

    NodeConnectionManagerViewUI.h
    Created: 16 Nov 2020 10:00:57am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "NodeConnectionViewUI.h"
#include "../NodeConnectionManager.h"

class NodeManagerViewUI;;

class NodeConnectionManagerViewUI :
    public BaseManagerUI<NodeConnectionManager, NodeConnection, NodeConnectionViewUI>
{
public:
    NodeConnectionManagerViewUI(NodeManagerViewUI * vui, NodeConnectionManager* manager);
    ~NodeConnectionManagerViewUI();

    NodeManagerViewUI* nodeManagerUI;

    NodeConnectionViewUI* createUIForItem(NodeConnection * item) override;

    void addItemUIInternal(NodeConnectionViewUI* ui) override;

    void resized();
};