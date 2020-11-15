/*
  ==============================================================================

    Node.h
    Created: 15 Nov 2020 8:40:03am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "NodeAudioProcessor.h"

class NodeViewUI;

class Node :
    public BaseItem
{
public:
    Node(StringRef name = "Node", var params = var());
    virtual ~Node();

    virtual void clearItem() override;

    AudioProcessorGraph::NodeID graphNodeID;
    NodeAudioProcessor * baseProcessor;
    AudioProcessorGraph::Node::Ptr nodeGraphPtr;

    StringArray audioInputNames;
    StringArray audioOutputNames;

    FloatParameter* outGain;

    void setProcessor(NodeAudioProcessor * processor); //needs to be called from

    void setAudioInputs(const StringArray & inputNames);
    void setAudioOutputs(const StringArray& outputNames);

    // Helpers
    bool hasAudioInput() { return !audioInputNames.isEmpty(); }
    bool hasAudioOutput() { return !audioOutputNames.isEmpty(); }

    virtual NodeViewUI* createViewUI();

    WeakReference<Node>::Master masterReference;
};

template<class T>
class GenericAudioNode :
    public Node
{
public:
    GenericAudioNode(var params = var()) :
        Node(getTypeString(), params)
    {
        processor = new T(this);
        setProcessor((GenericNodeAudioProcessor*)processor);
    }

    virtual ~GenericAudioNode() {}

    T* processor;

    virtual NodeViewUI* createViewUI() override 
    {
        return processor != nullptr ? ((GenericNodeAudioProcessor*)processor)->createNodeViewUI() : nullptr;
    }

    String getTypeString() const override { return T::getTypeStringStatic(); }
    static GenericAudioNode<T>* create(var params) { return new GenericAudioNode<T>(params); }
};

