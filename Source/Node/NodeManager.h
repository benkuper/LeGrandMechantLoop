/*
  ==============================================================================

    NodeManager.h
    Created: 15 Nov 2020 8:39:59am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

class NodeConnectionManager;
class NodeManagerAudioProcessor;

class AudioInputNode;
class AudioOutputNode;
class MIDIIONode;

class NodeManager :
    public BaseManager<Node>
{
public:
    NodeManager(AudioProcessorGraph* graph, AudioProcessorGraph::NodeID inputNodeID, AudioProcessorGraph::NodeID outputNodeID);
    ~NodeManager();
    
    FloatParameter* cpuUsage;

    BoolParameter* isPlaying;

    ControllableContainer looperControlCC;
    Trigger* stopAllLoopers;
    Trigger* playAllLoopers;
    Trigger* clearAllLoopers;
    Trigger* tmpMuteAllLoopers;
    Trigger* clearLastManipTrack;

    AudioProcessorGraph* graph;
    AudioProcessorGraph::NodeID inputNodeID;
    AudioProcessorGraph::NodeID outputNodeID;

    StringArray audioInputNames;
    StringArray audioOutputNames;

    Array<AudioInputNode*> audioInputNodes;
    Array<AudioOutputNode*> audioOutputNodes;
    Array<MIDIIONode*> midiIONodes;

    std::unique_ptr<NodeConnectionManager> connectionManager;

    void clear() override;

    void setAudioInputs(const int& numInputs); //auto naming
    void setAudioInputs(const StringArray& inputNames);
    void setAudioOutputs(const int& numOutputs); //auto naming
    void setAudioOutputs(const StringArray& outputNames);

    void updateAudioInputNode(AudioInputNode * n);
    void updateAudioOutputNode(AudioOutputNode* n);
    void updateMIDIIONode(MIDIIONode *n);

    virtual void addItemInternal(Node* n, var data) override;
    virtual void removeItemInternal(Node* n) override;

    virtual Array<UndoableAction*> getRemoveItemUndoableAction(Node* n) override;
    virtual Array<UndoableAction*> getRemoveItemsUndoableAction(Array<Node*> n) override;

    void onControllableFeedbackUpdate(ControllableContainer* cc, Controllable* c) override;

    bool hasPlayingNodes();
    
    var getJSONData() override;
    void loadJSONDataManagerInternal(var data) override;
};


class RootNodeManager :
    public NodeManager,
    public AudioManager::AudioManagerListener,
    public EngineListener,
    public Node::NodeListener
{
public:
    juce_DeclareSingleton(RootNodeManager, true);
    RootNodeManager();
    ~RootNodeManager();

    void audioSetupChanged() override;
    void onContainerParameterChanged(Parameter* p) override;

    void addItemInternal(Node* n, var data) override;
    void removeItemInternal(Node* n) override;

    void nodePlayConfigUpdated(Node * n) override;

    void endLoadFile() override;
};