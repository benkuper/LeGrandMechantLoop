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

class ContainerProcessor :
    public GenericNodeProcessor,
    public BaseManager<Node>::ManagerListener,
    public Node::NodeListener
{
public:
    ContainerProcessor(Node * n);
    ~ContainerProcessor();

    IntParameter* numAudioInputs;
    IntParameter* numAudioOutputs;

    FloatParameter* outVolume;//needs to be on genericProcessor level

    std::unique_ptr<NodeManager> nodeManager;
    AudioProcessorGraph graph;
    AudioProcessorGraph::NodeID inputID;
    AudioProcessorGraph::NodeID outputID;

    AudioBuffer<float> graphBuffer;

    void onContainerParameterChanged(Parameter* p) override;

    void itemAdded(Node* n) override;
    void itemRemoved(Node* n) override;

    void updateGraph();

    void updateInputsFromNodeInternal() override;
    void updateOutputsFromNodeInternal() override;

    void nodePlayConfigUpdated(Node* n) override;

    void updatePlayConfig() override;
    void prepareToPlay(double sampleRate, int maxBlockSize) override;
    void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

    static const String getTypeStringStatic() { return "Container"; }

    NodeViewUI* createNodeViewUI() override;
};