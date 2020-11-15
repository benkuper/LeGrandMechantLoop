/*
  ==============================================================================

    Node.cpp
    Created: 15 Nov 2020 8:40:03am
    Author:  bkupe

  ==============================================================================
*/

#include "Node.h"
#include "Engine/AudioManager.h"

Node::Node(StringRef name, Type type, NodeAudioProcessor * processor, var params) :
    BaseItem(name, true),
    type(type),
    graphNodeID(AudioManager::getInstance()->getNewGraphID()),
    audioProcessor(processor)
{
    nodeGraphPtr = AudioManager::getInstance()->graph.addNode(std::unique_ptr<NodeAudioProcessor>(audioProcessor), AudioProcessorGraph::NodeID(graphNodeID));
}

Node::~Node()
{
    if(nodeGraphPtr != nullptr) AudioManager::getInstance()->graph.removeNode(nodeGraphPtr.get());
}

void Node::clearItem()
{
    BaseItem::clearItem();
}

void Node::setAudioInputs(const StringArray& inputNames)
{
    audioInputNames = inputNames;
}

void Node::setAudioOutputs(const StringArray& outputNames)
{
    audioOutputNames = outputNames;
}

NodeAudioProcessor::NodeAudioProcessor(Node* n) :
    node(n)
{

}

NodeAudioProcessor::~NodeAudioProcessor()
{
}

void NodeAudioProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    if (node == nullptr)
    {
        buffer.clear();
        return;
    }
    
    if (!node->enabled->boolValue())
    {
        processBlockBypassed(buffer, midiMessages);
        return;
    }

    processBlockInternal(buffer, midiMessages);

}
