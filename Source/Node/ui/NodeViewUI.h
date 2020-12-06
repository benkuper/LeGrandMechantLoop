/*
  ==============================================================================

    NodeViewUI.h
    Created: 15 Nov 2020 9:26:57am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "../Node.h"
#include "Common/AudioUIHelpers.h"

class NodeConnector;

class BaseNodeViewUI:
    public BaseItemUI<Node>,
    public Node::AsyncListener
{
public:
    BaseNodeViewUI(Node* node);
    virtual ~BaseNodeViewUI();

    std::unique_ptr<NodeConnector> inAudioConnector;
    std::unique_ptr<NodeConnector> outAudioConnector;
    std::unique_ptr<NodeConnector> inMIDIConnector;
    std::unique_ptr<NodeConnector> outMIDIConnector;

    Rectangle<int> outControlRect;
    std::unique_ptr<FloatSliderUI> outGainUI;
    std::unique_ptr<RMSSliderUI> outRMSUI;

    void updateInputConnectors();
    void updateOutputConnectors();

    void paint(Graphics& g) override;
    void paintOverChildren(Graphics& g) override;
    virtual void resized() override;
    virtual void resizedInternalContent(Rectangle<int>& r) override;
    virtual void resizedInternalContentNode(Rectangle<int>& r) {}

    Rectangle<int> getMainBounds() override;

    virtual void nodeInputsChanged();
    virtual void nodeOutputsChanged();

    void newMessage(const Node::NodeEvent& e) override;
};

template<class T>
class NodeViewUI :
    public BaseNodeViewUI
{
public:
    NodeViewUI(T *node) : BaseNodeViewUI(node), node(node) {}
    ~NodeViewUI() {}
    T* node;
};