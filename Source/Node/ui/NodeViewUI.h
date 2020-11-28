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

class NodeViewUI :
    public BaseItemUI<Node>,
    public Node::AsyncListener
{
public:
    NodeViewUI(Node* node);
    virtual ~NodeViewUI();

    std::unique_ptr<NodeConnector> inAudioConnector;
    std::unique_ptr<NodeConnector> outAudioConnector;

    std::unique_ptr<RMSSliderUI> outRMSUI;

    void updateInputConnectors();
    void updateOutputConnectors();

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
class GenericAudioNodeViewUI :
    public NodeViewUI
{
public:
    GenericAudioNodeViewUI(GenericAudioNode<T>* audioNode) : NodeViewUI(audioNode), audioNode(audioNode), processor(audioNode->processor) {}
    ~GenericAudioNodeViewUI() {}

    GenericAudioNode<T> *  audioNode;
    T* processor;
};