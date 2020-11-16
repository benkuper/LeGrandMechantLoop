/*
  ==============================================================================

    NodeViewUI.h
    Created: 15 Nov 2020 9:26:57am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "../Node.h"

class NodeConnector;

class NodeViewUI :
    public BaseItemUI<Node>,
    public Node::AsyncListener
{
public:
    NodeViewUI(Node* node);
    virtual ~NodeViewUI();

    std::unique_ptr<NodeConnector> inAudioConnector;
    std::unique_ptr<NodeConnector> outAudioConnector;

    void updateInputConnectors();
    void updateOutputConnectors();

    virtual void resized();
    virtual void resizedInternalContent(Rectangle<int>& r) override;
    virtual void resizedInternalContentNode(Rectangle<int>& r) {}

    Rectangle<int> getMainBounds() override;

    virtual void nodeInputsChanged();
    virtual void nodeOutputsChanged();

    void newMessage(const Node::NodeEvent& e) override;
};

template<class T>
class GenericAudioNodeViewUI :
    public NodeViewUI
{
public:
    GenericAudioNodeViewUI(GenericAudioNode<T>* audioNode) : NodeViewUI(audioNode), audioNode(audioNode), processor(audioNode->processor) {}
    ~GenericAudioNodeViewUI() {}

    GenericAudioNode<T> *  audioNode;
    T* processor;
};