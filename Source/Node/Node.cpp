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
#include "Connection/NodeConnection.h"

Node::Node(StringRef name,var params) :
    BaseItem(name, true),
    nodeGraphID(AudioManager::getInstance()->getNewGraphID()),
    baseProcessor(nullptr),
    numInputs(0),
    numOutputs(0),
    hasMIDIInput(false),
    hasMIDIOutput(false),
    nodeNotifier(5)
{
    saveAndLoadRecursiveData = true;

    isNodePlaying = addBoolParameter("Is Node Playing", "This is a feedback to know if a node has playing content. Used by the global time to automatically stop if no content is playing", false);
    isNodePlaying->setControllableFeedbackOnly(true);
    isNodePlaying->hideInEditor = true;
}

Node::~Node()
{
    if(nodeGraphPtr != nullptr) AudioManager::getInstance()->graph.removeNode(nodeGraphPtr.get());
    masterReference.clear();
}

void Node::clearItem()
{
    baseProcessor->clearProcessor();
    BaseItem::clearItem();
}

void Node::setProcessor(NodeProcessor* processor)
{
    if (baseProcessor == processor) return;

    if (baseProcessor != nullptr)
    {
        removeChildControllableContainer(baseProcessor);
    }

    baseProcessor = processor;

    if (baseProcessor != nullptr)
    {
        nodeGraphPtr = AudioManager::getInstance()->graph.addNode(std::unique_ptr<NodeProcessor>(baseProcessor), AudioProcessorGraph::NodeID(nodeGraphID));
        baseProcessor->updateInputsFromNode(false);
        baseProcessor->updateOutputsFromNode(false);
        baseProcessor->updatePlayConfig();
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
    if (audioInputNames == inputNames) return;
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
    if (audioOutputNames == outputNames) return;
    audioOutputNames = outputNames;
    numOutputs = audioOutputNames.size();
    if (baseProcessor != nullptr) baseProcessor->updateOutputsFromNode();
    nodeListeners.call(&NodeListener::audioOutputsChanged, this);
    nodeNotifier.addMessage(new NodeEvent(NodeEvent::OUTPUTS_CHANGED, this));
}

void Node::addInConnection(NodeConnection* c)
{
    baseProcessor->suspendProcessing(true);
    if (c->connectionType == NodeConnection::AUDIO) inAudioConnections.addIfNotAlreadyThere((NodeAudioConnection*)c);
    else if (c->connectionType == NodeConnection::MIDI) inMidiConnections.addIfNotAlreadyThere((NodeMIDIConnection*)c);
    baseProcessor->suspendProcessing(false);
}

void Node::removeInConnection(NodeConnection* c)
{
    baseProcessor->suspendProcessing(true);
    if (c->connectionType == NodeConnection::AUDIO) inAudioConnections.removeAllInstancesOf((NodeAudioConnection*)c);
    else if (c->connectionType == NodeConnection::MIDI) inMidiConnections.removeAllInstancesOf((NodeMIDIConnection*)c);
    baseProcessor->suspendProcessing(false);
}

void Node::addOutConnection(NodeConnection* c)
{
    baseProcessor->suspendProcessing(true);
    if (c->connectionType == NodeConnection::AUDIO) outAudioConnections.addIfNotAlreadyThere((NodeAudioConnection*)c);
    else if (c->connectionType == NodeConnection::MIDI) outMidiConnections.addIfNotAlreadyThere((NodeMIDIConnection*)c);
    baseProcessor->suspendProcessing(false);
}

void Node::removeOutConnection(NodeConnection* c)
{
    baseProcessor->suspendProcessing(true);
    if (c->connectionType == NodeConnection::AUDIO) outAudioConnections.removeAllInstancesOf((NodeAudioConnection*)c);
    else if (c->connectionType == NodeConnection::MIDI) outMidiConnections.removeAllInstancesOf((NodeMIDIConnection*)c);
    baseProcessor->suspendProcessing(false);
}

void Node::setMIDIIO(bool hasInput, bool hasOutput)
{
    if (hasMIDIInput != hasInput)
    {
        hasMIDIInput = hasInput;
        nodeListeners.call(&NodeListener::midiInputChanged, this);
        nodeNotifier.addMessage(new NodeEvent(NodeEvent::MIDI_INPUT_CHANGED, this));
    }

    if (hasMIDIOutput != hasOutput)
    {
        hasMIDIOutput = hasOutput;
        nodeListeners.call(&NodeListener::midiOutputChanged, this);
        nodeNotifier.addMessage(new NodeEvent(NodeEvent::MIDI_OUTPUT_CHANGED, this));
    }

}

NodeViewUI* Node::createViewUI()
{
    return new NodeViewUI(this);
}
