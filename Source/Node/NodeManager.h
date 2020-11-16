/*
  ==============================================================================

    NodeManager.h
    Created: 15 Nov 2020 8:39:59am
    Author:  bkupe

  ==============================================================================
*/

#pragma once
#include "Node.h"
#include "Engine/AudioManager.h"

class NodeConnectionManager;
class NodeManagerAudioProcessor;

class NodeManager :
    public BaseManager<Node>
{
public:
    NodeManager(AudioProcessorGraph::NodeID inputNodeID, AudioProcessorGraph::NodeID outputNodeID);
    ~NodeManager();

    AudioProcessorGraph::NodeID inputNodeID;
    AudioProcessorGraph::NodeID outputNodeID;

    StringArray audioInputNames;
    StringArray audioOutputNames;

    Array<Node*> audioInputNodes;
    Array<Node*> audioOutputNodes;

    std::unique_ptr<NodeConnectionManager> connectionManager;

    void setAudioInputs(const int& numInputs); //auto naming
    void setAudioInputs(const StringArray& inputNames);
    void setAudioOutputs(const int& numOutputs); //auto naming
    void setAudioOutputs(const StringArray& outputNames);

    void updateAudioInputNode(Node * n);
    void updateAudioOutputNode(Node *n);

    virtual void addItemInternal(Node* n, var data) override;
    virtual void removeItemInternal(Node* n) override;

    var getJSONData() override;
    void loadJSONDataManagerInternal(var data) override;
};


class RootNodeManager :
    public NodeManager,
    public AudioManager::AudioManagerListener
{
public:
    juce_DeclareSingleton(RootNodeManager, true);
    RootNodeManager();
    ~RootNodeManager();

    void audioSetupChanged() override;
};

/*
class NodeManagerAudioProcessor :
    public AudioProcessor
{
public:
    NodeManagerAudioProcessor(NodeManager * manager);
    virtual ~NodeManagerAudioProcessor();

    NodeManager* nodeManager;
    WeakReference<Inspectable> nodeManagerRef;

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
*/