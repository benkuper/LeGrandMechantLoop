/*
  ==============================================================================

    NodeManager.cpp
    Created: 15 Nov 2020 8:39:59am
    Author:  bkupe

  ==============================================================================
*/

#include "NodeManager.h"
#include "NodeFactory.h"
#include "Engine/AudioManager.h"

juce_ImplementSingleton(RootNodeManager)

NodeManager::NodeManager(AudioProcessorGraph::NodeID inputNodeID, AudioProcessorGraph::NodeID outputNodeID, int numInputs, int numOutputs) :
    BaseManager("Nodes"),
    nodeGraphID(AudioManager::getInstance()->getNewGraphID()),
    inputNodeID(inputNodeID),
    outputNodeID(outputNodeID),
    numAudioInputs(0),
    numAudioOutputs(0)
{
     managerFactory = NodeFactory::getInstance();
     audioProcessor = new NodeManagerAudioProcessor(this);

     AudioManager::getInstance()->graph.addNode(std::unique_ptr<AudioProcessor>(audioProcessor), nodeGraphID);

     setNumInputChannels(numInputs);
     setNumOutputChannels(numOutputs);
}

NodeManager::~NodeManager()
{
}

void NodeManager::setNumInputChannels(int num)
{
    if (numAudioInputs == num) return;

    while (numAudioInputs > num) //remove surplus
    {
        numAudioInputs--;
        AudioManager::getInstance()->graph.removeConnection(AudioProcessorGraph::Connection({ inputNodeID, numAudioInputs }, { nodeGraphID, numAudioInputs })); //straight channel disconnection
    }
    
    while(numAudioInputs < num) //add missing
    {
        AudioManager::getInstance()->graph.addConnection(AudioProcessorGraph::Connection({ inputNodeID, numAudioInputs }, { nodeGraphID, numAudioInputs })); //straight channel connection
        numAudioInputs++;
    }
    
    //update graph ?
}

void NodeManager::setNumOutputChannels(int num)
{

    while (numAudioOutputs > num) //remove surplus
    {
        numAudioOutputs--;
        AudioManager::getInstance()->graph.removeConnection(AudioProcessorGraph::Connection({ nodeGraphID, numAudioOutputs }, { outputNodeID, numAudioOutputs })); //straight channel disconnection
    }

    while (numAudioOutputs < num) //add missing
    {
        AudioManager::getInstance()->graph.addConnection(AudioProcessorGraph::Connection({ nodeGraphID, numAudioOutputs }, { outputNodeID, numAudioOutputs })); //straight channel connection
        numAudioOutputs++;
    }
}

void NodeManager::addItemInternal(Node* n, var data)
{
}

void NodeManager::removeItemInternal(Node* n)
{
}

RootNodeManager::RootNodeManager() :
    NodeManager(AudioProcessorGraph::NodeID(AUDIO_GRAPH_INPUT_ID),
        AudioProcessorGraph::NodeID(AUDIO_GRAPH_OUTPUT_ID),
        AudioManager::getInstance()->am.getAudioDeviceSetup().inputChannels.countNumberOfSetBits(),
        AudioManager::getInstance()->am.getAudioDeviceSetup().outputChannels.countNumberOfSetBits())
{

}

RootNodeManager::~RootNodeManager()
{
}

NodeManagerAudioProcessor::NodeManagerAudioProcessor(NodeManager* manager) :
    nodeManager(manager),
    nodeManagerRef(manager)
{
}

NodeManagerAudioProcessor::~NodeManagerAudioProcessor()
{
}

void NodeManagerAudioProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
}
