/*
  ==============================================================================

    NodeManagerViewUI.h
    Created: 15 Nov 2020 8:40:22am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

class NodeConnectionManagerViewUI;

class NodeManagerViewUI :
    public BaseManagerViewUI<NodeManager, Node, BaseNodeViewUI>,
    public DragAndDropContainer
{
public:
    NodeManagerViewUI(NodeManager * manager);
    ~NodeManagerViewUI();

    NodeConnector* draggingConnector;
    Point<int> forcedDragTargetPos;

    std::unique_ptr< NodeConnectionManagerViewUI> connectionManagerUI;

    BaseNodeViewUI* createUIForItem(Node * n) override;

    void resized() override;

    void addItemUIInternal(BaseNodeViewUI* ui) override;

    void mouseDown(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;

    void paintOverChildren(Graphics& g) override;

    bool isInterestedInDragSource(const SourceDetails& details) override;
    void itemDragEnter(const DragAndDropTarget::SourceDetails& details) override;
    void itemDragMove(const DragAndDropTarget::SourceDetails& details) override;
    void itemDragExit(const DragAndDropTarget::SourceDetails& details) override;
    void itemDropped(const DragAndDropTarget::SourceDetails& details) override;

    void dragOperationEnded(const DragAndDropTarget::SourceDetails& details) override;

    NodeConnector * getCandidateConnector(bool lookForInput, NodeConnection::ConnectionType connectionType, BaseNodeViewUI* excludeUI = nullptr);

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