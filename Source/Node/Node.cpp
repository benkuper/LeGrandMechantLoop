/*
  ==============================================================================

    Node.cpp
    Created: 15 Nov 2020 8:40:03am
    Author:  bkupe

  ==============================================================================
*/

#include "Node.h"
#include "Engine/AudioManager.h"
#include "ui/NodeViewUI.h"

Node::Node(StringRef name,var params) :
    BaseItem(name, true),
    graphNodeID(AudioManager::getInstance()->getNewGraphID()),
    baseProcessor(nullptr)
{
    saveAndLoadRecursiveData = true;
}

Node::~Node()
{
    if(nodeGraphPtr != nullptr) AudioManager::getInstance()->graph.removeNode(nodeGraphPtr.get());
    masterReference.clear();
}

void Node::clearItem()
{
    BaseItem::clearItem();
}

void Node::setProcessor(NodeAudioProcessor* processor)
{
    if (baseProcessor == processor) return;

    if (baseProcessor != nullptr)
    {
        removeChildControllableContainer(baseProcessor);
    }

    baseProcessor = processor;

    if (baseProcessor != nullptr)
    {
        nodeGraphPtr = AudioManager::getInstance()->graph.addNode(std::unique_ptr<NodeAudioProcessor>(baseProcessor), AudioProcessorGraph::NodeID(graphNodeID));
        addChildControllableContainer(baseProcessor);
    }
}

void Node::setAudioInputs(const StringArray& inputNames)
{
    audioInputNames = inputNames;
}

void Node::setAudioOutputs(const StringArray& outputNames)
{
    audioOutputNames = outputNames;
}

NodeViewUI* Node::createViewUI()
{
    return new NodeViewUI(this);
}
