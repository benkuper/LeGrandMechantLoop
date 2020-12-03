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

ContainerProcessor::ContainerProcessor(Node* n) :
    GenericNodeProcessor(n),
    inputID(AudioProcessorGraph::NodeID(AudioManager::getInstance()->getNewGraphID())),
    outputID(AudioProcessorGraph::NodeID(AudioManager::getInstance()->getNewGraphID()))
{

    numAudioInputs = addIntParameter("Audio Inputs", "Number of audio inputs to create for this container", 2, 0, 16);
    numAudioOutputs = addIntParameter("Audio Outputs", "Number of audio outputs to create for this container", 2, 0, 16);

    std::unique_ptr<AudioProcessorGraph::AudioGraphIOProcessor> procIn(new AudioProcessorGraph::AudioGraphIOProcessor(AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode));
    std::unique_ptr<AudioProcessorGraph::AudioGraphIOProcessor> procOut(new AudioProcessorGraph::AudioGraphIOProcessor(AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode));

    graph.addNode(std::move(procIn), inputID);
    graph.addNode(std::move(procOut), outputID);

    nodeManager.reset(new NodeManager(&graph, inputID, outputID));
    addChildControllableContainer(nodeManager.get());

    nodeRef->setAudioInputs(numAudioInputs->intValue());
    nodeRef->setAudioOutputs(numAudioOutputs->intValue());
 
    nodeManager->addBaseManagerListener(this);
}

ContainerProcessor::~ContainerProcessor()
{
    nodeManager->clear();
}

void ContainerProcessor::onContainerParameterChanged(Parameter* p)
{
    if(p == numAudioInputs) nodeRef->setAudioInputs(numAudioInputs->intValue());
    else if(p == numAudioOutputs) nodeRef->setAudioOutputs(numAudioOutputs->intValue());
}

void ContainerProcessor::itemAdded(Node* n)
{
    n->addNodeListener(this);
    updateGraph();
}

void ContainerProcessor::itemRemoved(Node* n)
{
    n->removeNodeListener(this);
}

void ContainerProcessor::updateGraph()
{
    graph.suspendProcessing(true);
    graph.setPlayConfigDetails(nodeRef->numInputs, nodeRef->numOutputs, getSampleRate(), getBlockSize());
    graph.prepareToPlay(getSampleRate(), getBlockSize());
    graph.suspendProcessing(false);
}


void ContainerProcessor::updateInputsFromNodeInternal()
{
    nodeManager->setAudioInputs(nodeRef->audioInputNames);
}

void ContainerProcessor::updateOutputsFromNodeInternal()
{
    nodeManager->setAudioOutputs(nodeRef->audioOutputNames);
}

void ContainerProcessor::nodePlayConfigUpdated(Node* n)
{
    updateGraph();
}

void ContainerProcessor::updatePlayConfig()
{
    updateGraph();
    GenericNodeProcessor::updatePlayConfig();
}

void ContainerProcessor::prepareToPlay(double sampleRate, int maxBlockSize)
{
}

void ContainerProcessor::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
   // graphBuffer.makeCopyOf(buffer, true);
    //buffer.clear();
    graph.processBlock(buffer, midiMessages);

    //for (int i = 0; i < buffer.getNumChannels(); i++) buffer.copyFrom(i, 0, graphBuffer, i, 0, buffer.getNumSamples());
}

NodeViewUI* ContainerProcessor::createNodeViewUI()
{
    return new ContainerNodeViewUI(((GenericNode<ContainerProcessor>*)nodeRef.get()));
}
