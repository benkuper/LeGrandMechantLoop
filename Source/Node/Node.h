/*
  ==============================================================================

    Node.h
    Created: 15 Nov 2020 8:40:03am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "NodeProcessor.h"

class NodeConnection;
class NodeAudioConnection;
class NodeMIDIConnection;
class NodeViewUI;

class Node :
    public BaseItem
{
public:
    Node(StringRef name = "Node", var params = var());
    virtual ~Node();

    virtual void clearItem() override;

    AudioProcessorGraph* graph;

    BoolParameter* isNodePlaying;

    AudioProcessorGraph::NodeID nodeGraphID;
    NodeProcessor * baseProcessor;
    AudioProcessorGraph::Node::Ptr nodeGraphPtr;

    StringArray audioInputNames;
    StringArray audioOutputNames;

    int numInputs;
    int numOutputs;
    bool hasMIDIInput;
    bool hasMIDIOutput;

    Array<NodeAudioConnection*> inAudioConnections;
    Array<NodeAudioConnection*> outAudioConnections;
    Array<NodeMIDIConnection*> inMidiConnections;
    Array<NodeMIDIConnection*> outMidiConnections;

    void onContainerParameterChangedInternal(Parameter* p) override;

    void setGraph(AudioProcessorGraph* graph);
    void setProcessor(NodeProcessor * processor); //needs to be called from child classes

    void setAudioInputs(const int& numInputs); //auto naming
    void setAudioInputs(const StringArray & inputNames);
    void setAudioOutputs(const int& numOutputs); //auto naming
    void setAudioOutputs(const StringArray& outputNames);

    void addInConnection(NodeConnection* c);
    void removeInConnection(NodeConnection* c);
    void addOutConnection(NodeConnection* c);
    void removeOutConnection(NodeConnection* c);

    void setMIDIIO(bool hasInput, bool hasOutput);

    void notifyPlayConfigUpdated();

    template<class T>
    T* getProcessor() { return dynamic_cast<T*>(baseProcessor); }

    class NodeListener
    {
    public:
        virtual ~NodeListener() {}
        virtual void audioInputsChanged(Node *n) {}
        virtual void audioOutputsChanged(Node* n) {}
        virtual void midiInputChanged(Node* n) {}
        virtual void midiOutputChanged(Node * n) {}
        virtual void nodePlayConfigUpdated(Node* n) {};
    };

    ListenerList<NodeListener> nodeListeners;
    void addNodeListener(NodeListener* newListener) { nodeListeners.add(newListener); }
    void removeNodeListener(NodeListener* listener) { nodeListeners.remove(listener); }

    DECLARE_ASYNC_EVENT(Node, Node, node, ENUM_LIST(INPUTS_CHANGED, OUTPUTS_CHANGED, MIDI_INPUT_CHANGED, MIDI_OUTPUT_CHANGED))

    virtual NodeViewUI* createViewUI();
    WeakReference<Node>::Master masterReference;
};

template<class T>
class GenericNode :
    public Node
{
public:
    GenericNode(var params = var()) :
        Node(getTypeString(), params)
    {
        processor = new T(this);
        processor->suspendProcessing(true);
        setProcessor((GenericNodeProcessor*)processor);
        processor->suspendProcessing(false);
    }

    virtual ~GenericNode() {}

    T* processor;

    virtual NodeViewUI* createViewUI() override 
    {
        return processor != nullptr ? ((GenericNodeProcessor*)processor)->createNodeViewUI() : nullptr;
    }

    String getTypeString() const override { return T::getTypeStringStatic(); }
    static GenericNode<T>* create(var params) { return new GenericNode<T>(params); }
};

