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
    public BaseManagerViewUI<NodeManager, Node, NodeViewUI>
{
public:
    NodeManagerViewUI(NodeManager * manager);
    ~NodeManagerViewUI();

    std::unique_ptr< NodeConnectionManagerViewUI> connectionManagerUI;

    NodeViewUI* createUIForItem(Node * n) override;

    void resized() override;

    void mouseDown(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;

    NodeConnector * getCandidateConnector(bool lookForInput, NodeConnection::ConnectionType connectionType, NodeViewUI* excludeUI = nullptr);

};


class NodeManagerViewPanel :
    public ShapeShifterContentComponent,
    public Button::Listener
{
public:
    NodeManagerViewPanel(StringRef contentName);
    ~NodeManagerViewPanel();

    std::unique_ptr<NodeManagerViewUI> managerUI;

    OwnedArray<TextButton> crumbsBT;
    Array<NodeManager*> crumbManagers;

    void setManager(NodeManager* manager);
    void updateCrumbs();

    void resized() override;

    void buttonClicked(Button* b) override;

    static NodeManagerViewPanel* create(const String& name) { return new NodeManagerViewPanel(name); }

};