/*
  ==============================================================================

    NodeConnectionManagerViewUI.h
    Created: 16 Nov 2020 10:00:57am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

class NodeManagerViewUI;

class NodeConnectionManagerViewUI :
    public BaseManagerUI<NodeConnectionManager, NodeConnection, NodeConnectionViewUI>
{
public:
    NodeConnectionManagerViewUI(NodeManagerViewUI * vui, NodeConnectionManager* manager);
    ~NodeConnectionManagerViewUI();

    NodeManagerViewUI* nodeManagerUI;
    std::unique_ptr<NodeConnectionViewUI> tmpConnectionUI;
    NodeConnection * tmpConnectionToReplace;
    bool tmpConnectionLookForInput;

    NodeConnectionViewUI* createUIForItem(NodeConnection * item) override;

    void placeItems(Rectangle<int>& r) override;

    void addItemUIInternal(NodeConnectionViewUI* ui) override;

    void resized() override; //override to avoid baseManagerUI stuff

    void startCreateConnection(NodeConnector* connector, NodeConnection * connectionToReplace = nullptr);
    void updateCreateConnection();
    void endCreateConnection();
};