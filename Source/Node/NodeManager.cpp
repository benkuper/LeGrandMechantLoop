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
#include "Connection/NodeConnectionManager.h"
#include "nodes/io/IONode.h"

juce_ImplementSingleton(RootNodeManager)

NodeManager::NodeManager(AudioProcessorGraph::NodeID inputNodeID, AudioProcessorGraph::NodeID outputNodeID) :
	BaseManager("Nodes"),
	//nodeGraphID(AudioManager::getInstance()->getNewGraphID()),
	inputNodeID(inputNodeID),
	outputNodeID(outputNodeID)
{
	managerFactory = NodeFactory::getInstance();
	//audioProcessor = new NodeManagerAudioProcessor(this);
	//AudioManager::getInstance()->graph.addNode(std::unique_ptr<AudioProcessor>(audioProcessor), nodeGraphID);

	connectionManager.reset(new NodeConnectionManager());
	addChildControllableContainer(connectionManager.get());

}

NodeManager::~NodeManager()
{
}


void NodeManager::setAudioInputs(const int& numInputs)
{
	StringArray s;
	for (int i = 0; i < numInputs; i++) s.add("Input " + String(i + 1));
	setAudioInputs(s);
}

void NodeManager::setAudioInputs(const StringArray& inputNames)
{
	/*
	int currentNumInputs = audioInputNames.size();
	int newNumInputs = inputNames.size();
	while (currentNumInputs > newNumInputs) //remove surplus
	{
		currentNumInputs--;
		AudioManager::getInstance()->graph.removeConnection(AudioProcessorGraph::Connection({ inputNodeID, currentNumInputs }, { nodeGraphID, currentNumInputs })); //straight channel disconnection
	}

	while (currentNumInputs < newNumInputs) //add missing
	{
		AudioManager::getInstance()->graph.addConnection(AudioProcessorGraph::Connection({ inputNodeID, currentNumInputs }, { nodeGraphID, currentNumInputs })); //straight channel connection
		currentNumInputs++;
	}
	*/

	audioInputNames = inputNames;
	for (auto& n : audioInputNodes) updateAudioInputNode(n);

}

void NodeManager::setAudioOutputs(const int& numOutputs)
{
	StringArray s;
	for (int i = 0; i < numOutputs; i++) s.add("Output " + String(i + 1));
	setAudioOutputs(s);
}

void NodeManager::setAudioOutputs(const StringArray& outputNames)
{
	/*
	int currentNumOutputs = audioOutputNames.size();
	int newNumOutputs = outputNames.size();
	while (currentNumOutputs > newNumOutputs) //remove surplus
	{
		currentNumOutputs--;
		AudioManager::getInstance()->graph.removeConnection(AudioProcessorGraph::Connection({ nodeGraphID, currentNumOutputs }, { outputNodeID, currentNumOutputs })); //straight channel disconnection
	}

	while (currentNumOutputs < newNumOutputs) //add missing
	{
		AudioManager::getInstance()->graph.addConnection(AudioProcessorGraph::Connection({ nodeGraphID, currentNumOutputs }, { outputNodeID, currentNumOutputs })); //straight channel connection
		currentNumOutputs++;
	}
	*/

	audioOutputNames = outputNames;
	for (auto& n : audioOutputNodes) updateAudioOutputNode(n);

}


void NodeManager::updateAudioInputNode(Node* n)
{
	if (AudioInputProcessor* p = n->getProcessor<AudioInputProcessor>())
	{
		for (int i = 0; i < n->audioInputNames.size(); i++) AudioManager::getInstance()->graph.removeConnection(AudioProcessorGraph::Connection({ inputNodeID, i }, { n->nodeGraphID, i })); //straight channel 
		n->setAudioOutputs(audioInputNames); //inverse to get good connector names
		for (int i = 0; i < audioInputNames.size(); i++) AudioManager::getInstance()->graph.addConnection(AudioProcessorGraph::Connection({ inputNodeID, i }, { n->nodeGraphID, i })); //straight 
	}
}

void NodeManager::updateAudioOutputNode(Node* n)
{
	if (AudioOutputProcessor* p = n->getProcessor<AudioOutputProcessor>())
	{
		for (int i = 0; i < n->audioOutputNames.size(); i++) AudioManager::getInstance()->graph.removeConnection(AudioProcessorGraph::Connection({ n->nodeGraphID, i }, { outputNodeID, i })); //straight channel 
		n->setAudioInputs(audioOutputNames); //inverse to get good connector names
		for (int i = 0; i < audioOutputNames.size(); i++) AudioManager::getInstance()->graph.addConnection(AudioProcessorGraph::Connection({ n->nodeGraphID, i }, { outputNodeID, i })); //straight 
	}
}

void NodeManager::addItemInternal(Node* n, var data)
{
	if (AudioInputProcessor* p = n->getProcessor<AudioInputProcessor>())
	{
		audioInputNodes.add(n);
		updateAudioInputNode(n);
	}
	else if (AudioOutputProcessor* p = n->getProcessor<AudioOutputProcessor>())
	{
		audioOutputNodes.add(n);
		updateAudioOutputNode(n);
	}
}

void NodeManager::removeItemInternal(Node* n)
{
	if (AudioInputProcessor* p = n->getProcessor<AudioInputProcessor>())  audioInputNodes.removeAllInstancesOf(n);
	else if (AudioOutputProcessor* p = n->getProcessor<AudioOutputProcessor>()) audioOutputNodes.removeAllInstancesOf(n);
}

Array<UndoableAction*> NodeManager::getRemoveItemUndoableAction(Node* item)
{
	Array<UndoableAction*> result;
	Array<Node*> itemsToRemove;
	itemsToRemove.add(item);
	result.addArray(connectionManager->getRemoveAllLinkedConnectionsActions(itemsToRemove));
	result.addArray(BaseManager::getRemoveItemUndoableAction(item));
	return result;
}

Array<UndoableAction*> NodeManager::getRemoveItemsUndoableAction(Array<Node*> itemsToRemove)
{
	Array<UndoableAction*> result;
	result.addArray(connectionManager->getRemoveAllLinkedConnectionsActions(itemsToRemove));
	result.addArray(BaseManager::getRemoveItemsUndoableAction(itemsToRemove));
	return result;
}

var NodeManager::getJSONData()
{
	var data = BaseManager::getJSONData();
	data.getDynamicObject()->setProperty(connectionManager->shortName, connectionManager->getJSONData());
	return data;
}

void NodeManager::loadJSONDataManagerInternal(var data)
{
	BaseManager::loadJSONDataManagerInternal(data);
	connectionManager->loadJSONData(data.getProperty(connectionManager->shortName, var()));
}


//ROOT

RootNodeManager::RootNodeManager() :
	NodeManager(AudioProcessorGraph::NodeID(AUDIO_GRAPH_INPUT_ID),
		AudioProcessorGraph::NodeID(AUDIO_GRAPH_OUTPUT_ID))
{
	AudioManager::getInstance()->addAudioManagerListener(this);

	setAudioInputs(AudioManager::getInstance()->getInputChannelNames());
	setAudioOutputs(AudioManager::getInstance()->getOutputChannelNames());
}

RootNodeManager::~RootNodeManager()
{
	AudioManager::getInstance()->removeAudioManagerListener(this);
}

void RootNodeManager::audioSetupChanged()
{
	setAudioInputs(AudioManager::getInstance()->getInputChannelNames());
	setAudioOutputs(AudioManager::getInstance()->getOutputChannelNames());
}

/*
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
	DBG("Process block");
}
*/