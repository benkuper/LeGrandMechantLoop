/*
  ==============================================================================

    ContainerNode.h
    Created: 3 Dec 2020 9:42:13am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "../../Node.h"
class NodeManager;

class ContainerNode :
    public Node,
    public BaseManager<Node>::ManagerListener,
    public Node::NodeListener
{
public:
    ContainerNode(var params = var());
    ~ContainerNode();

    std::unique_ptr<NodeManager> nodeManager;
    AudioProcessorGraph containerGraph;
    AudioProcessorGraph::NodeID inputID;
    AudioProcessorGraph::NodeID outputID;

    AudioBuffer<float> graphBuffer;

    void clearItem() override;

    void itemAdded(Node* n) override;
    void itemRemoved(Node* n) override;

    void updateGraph();

    void updateAudioInputsInternal() override;
    void updateAudioOutputsInternal() override;

    void nodePlayConfigUpdated(Node* n) override;
    
    void onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c) override;

    void updatePlayConfigInternal() override;
    void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;
    
    var getJSONData() override;
    void loadJSONDataInternal(var data) override;

    String getTypeString() const override { return getTypeStringStatic(); }
    static const String getTypeStringStatic() { return "Container"; }

    BaseNodeViewUI* createViewUI() override;
};