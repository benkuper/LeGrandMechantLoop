/*
  ==============================================================================

    NodeConnectionViewUI.h
    Created: 16 Nov 2020 10:01:02am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

class NodeConnector;

class NodeConnectionViewUI :
    public BaseItemMinimalUI<NodeConnection>,
    public ComponentListener,
    public NodeConnection::AsyncListener,
    public Timer
{
public:
    NodeConnectionViewUI(NodeConnection* connection, NodeConnector * sourceConnector = nullptr, NodeConnector * destConnector = nullptr);
    ~NodeConnectionViewUI();

    NodeConnector* sourceConnector;
    NodeConnector* destConnector;

    Path path;
    Path hitPath;

    float activityLevel;

    void paint(Graphics& g) override;

    void updateBounds();

    void buildPath();

    bool hitTest(int x, int y) override;

    void setSourceConnector(NodeConnector* c);
    void setDestConnector(NodeConnector* c);

    virtual void mouseUp(const MouseEvent& e) override;

    virtual bool isInterestedInDragSource(const SourceDetails& details) override;
    virtual void itemDragEnter(const SourceDetails& details) override;
    virtual void itemDragExit(const SourceDetails& details) override;
    virtual void itemDropped(const SourceDetails& details) override;

    void componentMovedOrResized(Component& c, bool, bool) override;
    void componentBeingDeleted(Component& c) override;

    void newMessage(const NodeConnection::ConnectionEvent& e) override;

    //class Handle :
    //    public Component
    //{
    //public:
    //    Handle(Colour c);
    //    ~Handle() {}
    //    Colour color;
    //    void paint(Graphics& g) override;
    //};

    //Handle sourceHandle;
    //Handle destHandle;

    void timerCallback() override;
};