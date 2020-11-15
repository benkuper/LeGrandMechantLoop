/*
  ==============================================================================

    NodeManager.h
    Created: 15 Nov 2020 8:39:59am
    Author:  bkupe

  ==============================================================================
*/

#pragma once
#include "Node.h"

class NodeManagerAudioProcessor;

class NodeManager :
    public BaseManager<Node>
{
public:
    NodeManager(AudioProcessorGraph::NodeID inputNodeID, AudioProcessorGraph::NodeID outputNodeID, int numInputs, int numOutputs);
    ~NodeManager();

    AudioProcessorGraph::NodeID nodeGraphID;
    NodeManagerAudioProcessor* audioProcessor;
    AudioProcessorGraph::Node::Ptr nodeGraphPtr;

    AudioProcessorGraph::NodeID inputNodeID;
    AudioProcessorGraph::NodeID outputNodeID;

    int numAudioInputs;
    int numAudioOutputs;

    void setNumInputChannels(int num);
    void setNumOutputChannels(int num);

    virtual void addItemInternal(Node* n, var data) override;
    virtual void removeItemInternal(Node* n) override;
};


class RootNodeManager :
    public NodeManager
{
public:
    juce_DeclareSingleton(RootNodeManager, true);
    RootNodeManager();
    ~RootNodeManager();
};


class NodeManagerAudioProcessor :
    public AudioProcessor
{
public:
    NodeManagerAudioProcessor(NodeManager * manager);
    virtual ~NodeManagerAudioProcessor();

    NodeManager* nodeManager;
    WeakReference<Inspectable> nodeManagerRef;

    /* AudioProcessor */
    virtual const String getName() const override { return nodeManager == nullptr ? "[Deleted]" : nodeManager->niceName; }
    virtual void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) {}
    virtual void releaseResources() override {}
    virtual void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;
    virtual double getTailLengthSeconds() const override { return 0; }
    virtual bool acceptsMidi() const override { return false; }
    virtual bool producesMidi() const override { return false; }

    virtual AudioProcessorEditor* createEditor() override { return nullptr; }
    virtual bool hasEditor() const override { return false; }
    virtual int getNumPrograms() override { return 0; }
    virtual int getCurrentProgram() override { return 0; }
    virtual void setCurrentProgram(int index) override {}
    virtual const String getProgramName(int index) override { return "[NoProgram]"; }
    virtual void changeProgramName(int index, const String& newName) override {}
    virtual void getStateInformation(juce::MemoryBlock& destData) override {}
    virtual void setStateInformation(const void* data, int sizeInBytes) override {}
};