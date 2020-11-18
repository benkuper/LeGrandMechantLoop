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
    std::unique_ptr<NodeConnectionViewUI> tmpConnectionUI;
    bool tmpConnectionLookForInput;

    NodeConnectionViewUI* createUIForItem(NodeConnection * item) override;

    void placeItems(Rectangle<int>& r) override;

    void addItemUIInternal(NodeConnectionViewUI* ui) override;

    void resized(); //override to avoid baseManagerUI stuff

    void startCreateConnection(NodeConnector* connector);
    void updateCreateConnection();
    void endCreateConnection();
};