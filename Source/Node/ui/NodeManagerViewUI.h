/*
  ==============================================================================

    NodeManagerViewUI.h
    Created: 15 Nov 2020 8:40:22am
    Author:  bkupe

  ==============================================================================
*/

#pragma once
#include "../NodeManager.h"
#include "NodeViewUI.h"
#include "../Connection/NodeConnection.h"

class NodeConnectionManagerViewUI;

class NodeManagerViewUI :
    public BaseManagerShapeShifterViewUI<NodeManager, Node, NodeViewUI>
{
public:
    NodeManagerViewUI(StringRef name);
    ~NodeManagerViewUI();

    std::unique_ptr< NodeConnectionManagerViewUI> connectionManagerUI;

    NodeViewUI* createUIForItem(Node * n) override;

    void resized() override;

    void mouseDown(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;

    NodeConnector * getCandidateConnector(bool lookForInput, NodeConnection::ConnectionType connectionType, NodeViewUI* excludeUI = nullptr);

    static NodeManagerViewUI* create(const String& name) { return new NodeManagerViewUI(name); }
};
