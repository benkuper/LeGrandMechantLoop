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
#include "Transport/Transport.h"

juce_ImplementSingleton(RootNodeManager)

NodeManager::NodeManager(AudioProcessorGraph* graph, AudioProcessorGraph::NodeID inputNodeID, AudioProcessorGraph::NodeID outputNodeID) :
	BaseManager("Nodes"),
	graph(graph),
	inputNodeID(inputNodeID),
	outputNodeID(outputNodeID)
{
	managerFactory = NodeFactory::getInstance();
	
	isPlaying = addBoolParameter("Is Node Playing", "This is a feedback to know if a node has playing content. Used by the global time to automatically stop if no content is playing", false);
	isPlaying->setControllableFeedbackOnly(true);
	isPlaying->hideInEditor = true;

	connectionManager.reset(new NodeConnectionManager(this));
	addChildControllableContainer(connectionManager.get());
}



NodeManager::~NodeManager()
{
}


void NodeManager::clear()
{
	connectionManager->clear();
	BaseManager::clear();
}

void NodeManager::setAudioInputs(const int& numInputs)
{
	StringArray s;
	for (int i = 0; i < numInputs; i++) s.add("Input " + String(i + 1));
	setAudioInputs(s);
}

void NodeManager::setAudioInputs(const StringArray& inputNames)
{
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
	audioOutputNames = outputNames;
	for (auto& n : audioOutputNodes) updateAudioOutputNode(n);
}


void NodeManager::updateAudioInputNode(Node* n)
{
	if (AudioInputProcessor* p = n->getProcessor<AudioInputProcessor>())
	{
		for (int i = 0; i < n->audioInputNames.size(); i++) graph->removeConnection(AudioProcessorGraph::Connection({ inputNodeID, i }, { n->nodeGraphID, i })); //straight channel 
		n->setAudioOutputs(audioInputNames); //inverse to get good connector names
		for (int i = 0; i < audioInputNames.size(); i++) graph->addConnection(AudioProcessorGraph::Connection({ inputNodeID, i }, { n->nodeGraphID, i })); //straight 
	}
}

void NodeManager::updateAudioOutputNode(Node* n)
{
	if (AudioOutputProcessor* p = n->getProcessor<AudioOutputProcessor>())
	{
		for (int i = 0; i < n->audioOutputNames.size(); i++) graph->removeConnection(AudioProcessorGraph::Connection({ n->nodeGraphID, i }, { outputNodeID, i })); //straight channel 
		n->setAudioInputs(audioOutputNames); //inverse to get good connector names
		for (int i = 0; i < audioOutputNames.size(); i++) graph->addConnection(AudioProcessorGraph::Connection({ n->nodeGraphID, i }, { outputNodeID, i })); //straight 
	}
}

void NodeManager::addItemInternal(Node* n, var data)
{
	n->setGraph(graph);

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


void NodeManager::onControllableFeedbackUpdate(ControllableContainer* cc, Controllable* c)
{
	if (Node* n = c->getParentAs<Node>())
	{
		if (c == n->isNodePlaying)
		{
			isPlaying->setValue(hasPlayingNodes());
		}
	}
}

bool NodeManager::hasPlayingNodes()
{
	for (auto& i : items) if (i->isNodePlaying->boolValue()) return true;
	return false;
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
	NodeManager(&AudioManager::getInstance()->graph,
		AudioProcessorGraph::NodeID(AUDIO_GRAPH_INPUT_ID),
		AudioProcessorGraph::NodeID(AUDIO_GRAPH_OUTPUT_ID))
{
	AudioManager::getInstance()->addAudioManagerListener(this);

	setAudioInputs(AudioManager::getInstance()->getInputChannelNames());
	setAudioOutputs(AudioManager::getInstance()->getOutputChannelNames());

	Engine::mainEngine->addEngineListener(this);
}

RootNodeManager::~RootNodeManager()
{
	Engine::mainEngine->removeEngineListener(this);
	AudioManager::getInstance()->removeAudioManagerListener(this);
}

void RootNodeManager::audioSetupChanged()
{
	setAudioInputs(AudioManager::getInstance()->getInputChannelNames());
	setAudioOutputs(AudioManager::getInstance()->getOutputChannelNames());
}

void RootNodeManager::onContainerParameterChanged(Parameter* p)
{
	NodeManager::onContainerParameterChanged(p);

	if (p == isPlaying)
	{
		if (!isPlaying->boolValue())
		{
			Transport::getInstance()->stopTrigger->trigger();
		}
	}
}

void RootNodeManager::addItemInternal(Node* n, var data)
{
	NodeManager::addItemInternal(n, data);
	n->addNodeListener(this);
	AudioManager::getInstance()->updateGraph();
}

void RootNodeManager::removeItemInternal(Node* n)
{
	NodeManager::removeItemInternal(n);
	n->removeNodeListener(this);
}

void RootNodeManager::nodePlayConfigUpdated(Node* n)
{
	AudioManager::getInstance()->updateGraph();
}

void RootNodeManager::endLoadFile()
{
	for (auto& i : items) i->baseProcessor->updatePlayConfig();
}
