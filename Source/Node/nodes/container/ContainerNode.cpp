/*
  ==============================================================================

    ContainerNode.cpp
    Created: 3 Dec 2020 9:42:13am
    Author:  bkupe

  ==============================================================================
*/

#include "Node/NodeIncludes.h"

ContainerNode::ContainerNode(var params) :
    Node(getTypeString(), params, true, true, true, true),
    inputID(AudioProcessorGraph::NodeID(AudioManager::getInstance()->getNewGraphID())),
    outputID(AudioProcessorGraph::NodeID(AudioManager::getInstance()->getNewGraphID()))
{
    iNode = new AudioProcessorGraph::AudioGraphIOProcessor(AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode);
    oNode = new AudioProcessorGraph::AudioGraphIOProcessor(AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode);
    containerGraph.addNode(std::unique_ptr< AudioProcessorGraph::AudioGraphIOProcessor>(iNode), inputID);
    containerGraph.addNode(std::unique_ptr< AudioProcessorGraph::AudioGraphIOProcessor>(oNode), outputID);


    nodeManager.reset(new NodeManager(&containerGraph, inputID, outputID));
    addChildControllableContainer(nodeManager.get());

    nodeManager->addBaseManagerListener(this);

    viewUISize->setPoint(200, 150);
}

ContainerNode::~ContainerNode()
{
}

void ContainerNode::clearItem()
{
    Node::clearItem();
    nodeManager->clear();
}

void ContainerNode::initInternal()
{
    updateGraph();
}

void ContainerNode::itemAdded(Node* n)
{
    n->addNodeListener(this);
    if (!isCurrentlyLoadingData) updateGraph();
}

void ContainerNode::itemRemoved(Node* n)
{
    n->removeNodeListener(this);
}

void ContainerNode::updateGraph()
{
    ScopedSuspender sp(processor);
    
    iNode->setPlayConfigDetails(getNumAudioInputs(), getNumAudioInputs(), processor->getSampleRate(), processor->getBlockSize());
    oNode->setPlayConfigDetails(getNumAudioOutputs(), getNumAudioOutputs(), processor->getSampleRate(), processor->getBlockSize());
    containerGraph.setPlayConfigDetails(getNumAudioInputs(), getNumAudioOutputs(), processor->getSampleRate(), processor->getBlockSize());
    containerGraph.prepareToPlay(processor->getSampleRate(), processor->getBlockSize());
}

void ContainerNode::updateAudioInputsInternal()
{
    nodeManager->setAudioInputs(audioInputNames);
}

void ContainerNode::updateAudioOutputsInternal()
{
    nodeManager->setAudioOutputs(audioOutputNames);
}

void ContainerNode::nodePlayConfigUpdated(Node* n)
{
    if (!isCurrentlyLoadingData) updateGraph();
}

void ContainerNode::onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c)
{
    if (c == nodeManager->isPlaying)
    {
        isNodePlaying->setValue(nodeManager->isPlaying->boolValue());
    }
    Node::onControllableFeedbackUpdateInternal(cc, c);
}

void ContainerNode::updatePlayConfigInternal()
{
    Node::updatePlayConfigInternal();
    if(!isCurrentlyLoadingData) updateGraph();
}


void ContainerNode::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    containerGraph.processBlock(buffer, midiMessages);
}

var ContainerNode::getJSONData()
{
    var data = Node::getJSONData();
    data.getDynamicObject()->setProperty("manager", nodeManager->getJSONData());
    return data;
}

void ContainerNode::loadJSONDataItemInternal(var data)
{
    Node::loadJSONDataItemInternal(data);
    updateGraph();
    nodeManager->loadJSONData(data.getProperty("manager", var()));
}


BaseNodeViewUI* ContainerNode::createViewUI()
{
    return new ContainerNodeViewUI(this);
}
