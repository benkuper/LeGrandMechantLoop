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
    nodeGraphID(AudioManager::getInstance()->getNewGraphID()),
    baseProcessor(nullptr),
    numInputs(0),
    numOutputs(0),
    nodeNotifier(5)
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
        nodeGraphPtr = AudioManager::getInstance()->graph.addNode(std::unique_ptr<NodeAudioProcessor>(baseProcessor), AudioProcessorGraph::NodeID(nodeGraphID));
        addChildControllableContainer(baseProcessor);
    }
}

void Node::setAudioInputs(const int& numInputs)
{
    StringArray s;
    for (int i = 0; i < numInputs; i++) s.add("Input " + String(i + 1));
    setAudioInputs(s);
}

void Node::setAudioInputs(const StringArray& inputNames)
{
    audioInputNames = inputNames;
    numInputs = audioInputNames.size();
    if (baseProcessor != nullptr) baseProcessor->updateInputsFromNode();
    nodeListeners.call(&NodeListener::audioInputsChanged, this);
    nodeNotifier.addMessage(new NodeEvent(NodeEvent::INPUTS_CHANGED, this));
}

void Node::setAudioOutputs(const int& numOutputs)
{
    StringArray s;
    for (int i = 0; i < numOutputs; i++) s.add("Output " + String(i + 1));
    setAudioOutputs(s);
}

void Node::setAudioOutputs(const StringArray& outputNames)
{
    audioOutputNames = outputNames;
    numOutputs = audioOutputNames.size();
    if (baseProcessor != nullptr) baseProcessor->updateOutputsFromNode();
    nodeListeners.call(&NodeListener::audioOutputsChanged, this);
    nodeNotifier.addMessage(new NodeEvent(NodeEvent::OUTPUTS_CHANGED, this));
}

NodeViewUI* Node::createViewUI()
{
    return new NodeViewUI(this);
}
