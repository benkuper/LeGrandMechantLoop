/*
  ==============================================================================

    ContainerNode.cpp
    Created: 3 Dec 2020 9:42:13am
    Author:  bkupe

  ==============================================================================
*/

#include "ContainerNode.h"
#include "../../NodeManager.h"
#include "ui/ContainerNodeUI.h"

ContainerNode::ContainerNode(var params) :
    Node(getTypeString(), params, true, true, true, true),
    inputID(AudioProcessorGraph::NodeID(AudioManager::getInstance()->getNewGraphID())),
    outputID(AudioProcessorGraph::NodeID(AudioManager::getInstance()->getNewGraphID()))
{
    std::unique_ptr<AudioProcessorGraph::AudioGraphIOProcessor> procIn(new AudioProcessorGraph::AudioGraphIOProcessor(AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode));
    std::unique_ptr<AudioProcessorGraph::AudioGraphIOProcessor> procOut(new AudioProcessorGraph::AudioGraphIOProcessor(AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode));

    containerGraph.addNode(std::move(procIn), inputID);
    containerGraph.addNode(std::move(procOut), outputID);

    nodeManager.reset(new NodeManager(&containerGraph, inputID, outputID));
    addChildControllableContainer(nodeManager.get());

    nodeManager->addBaseManagerListener(this);
}

ContainerNode::~ContainerNode()
{
}

void ContainerNode::clearItem()
{
    Node::clearItem();
    nodeManager->clear();
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
    containerGraph.setPlayConfigDetails(getNumAudioInputs(), getNumAudioOutputs(), graph->getSampleRate(), graph->getBlockSize());
    containerGraph.prepareToPlay(graph->getSampleRate(), graph->getBlockSize());
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
    if (c == nodeManager->isPlaying) isNodePlaying->setValue(nodeManager->isPlaying->boolValue());
    Node::onControllableFeedbackUpdateInternal(cc, c);
}

void ContainerNode::updatePlayConfigInternal()
{
    Node::updatePlayConfigInternal();
    updateGraph();
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

void ContainerNode::loadJSONDataInternal(var data)
{
    nodeManager->loadJSONData(data.getProperty("manager", var()));
    Node::loadJSONDataInternal(data);
}

BaseNodeViewUI* ContainerNode::createViewUI()
{
    return new ContainerNodeViewUI(this);
}
