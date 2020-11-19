/*
  ==============================================================================

    Node.h
    Created: 15 Nov 2020 8:40:03am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "NodeAudioProcessor.h"

#
class NodeConnection;
class NodeViewUI;

class Node :
    public BaseItem
{
public:
    Node(StringRef name = "Node", var params = var());
    virtual ~Node();

    virtual void clearItem() override;

    AudioProcessorGraph::NodeID nodeGraphID;
    NodeAudioProcessor * baseProcessor;
    AudioProcessorGraph::Node::Ptr nodeGraphPtr;

    StringArray audioInputNames;
    StringArray audioOutputNames;

    int numInputs;
    int numOutputs;

    Array<NodeConnection*> inConnections;
    Array<NodeConnection*> outConnections;

    FloatParameter* outGain;

    void setProcessor(NodeAudioProcessor * processor); //needs to be called from

    void setAudioInputs(const int& numInputs); //auto naming
    void setAudioInputs(const StringArray & inputNames);
    void setAudioOutputs(const int& numOutputs); //auto naming
    void setAudioOutputs(const StringArray& outputNames);

    // Helpers
    virtual bool exposeAudioInput() { return baseProcessor->exposeAudioInput(); }
    virtual bool exposeAudioOutput() { return baseProcessor->exposeAudioOutput(); }


    template<class T>
    T* getProcessor() { return dynamic_cast<T*>(baseProcessor); }


    class NodeListener
    {
    public:
        virtual ~NodeListener() {}
        virtual void audioInputsChanged(Node *n) {}
        virtual void audioOutputsChanged(Node * n) {}
    };

    ListenerList<NodeListener> nodeListeners;
    void addNodeListener(NodeListener* newListener) { nodeListeners.add(newListener); }
    void removeNodeListener(NodeListener* listener) { nodeListeners.remove(listener); }

    DECLARE_ASYNC_EVENT(Node, Node, node, ENUM_LIST(INPUTS_CHANGED, OUTPUTS_CHANGED))

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

