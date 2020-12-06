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
#include "Transport/Transport.h"

Node::Node(StringRef name, var params, bool hasInput, bool hasOutput, bool userCanSetIO, bool useOutControl) :
	BaseItem(name, true),
	graph(nullptr),
	nodeGraphID(AudioManager::getInstance()->getNewGraphID()),
	hasMIDIInput(false),
	hasMIDIOutput(false),
	hasAudioInput(hasAudioInput),
	hasAudioOutput(hasAudioOutput),
	numAudioInputs(nullptr),
	numAudioOutputs(nullptr),
	outGain(nullptr),
	outRMS(nullptr),
	nodeNotifier(5)
{
	processor = new NodeAudioProcessor(this);
	processor->setPlayHead(Transport::getInstance());

	saveAndLoadRecursiveData = true;

	isNodePlaying = addBoolParameter("Is Node Playing", "This is a feedback to know if a node has playing content. Used by the global time to automatically stop if no content is playing", false);
	isNodePlaying->setControllableFeedbackOnly(true);
	isNodePlaying->hideInEditor = true;

	if (userCanSetIO)
	{
		if (hasAudioInput)
		{
			numAudioInputs = addIntParameter("Audio Inputs", "Number of audio inputs for this node", 2, 0, 32);
		}

		if (hasAudioOutput)
		{
			numAudioOutputs = addIntParameter("Audio Outputs", "Number of audio outputs for this node", 2, 0, 32);
		}
	}

	if (useOutControl)
	{
		outGain = addFloatParameter("Out Gain", "Gain to apply to all channels after processing", 1, 0, 2);
		outRMS = addFloatParameter("Out RMS", "The general activity of all channels combined after processing", 0, 0, 1);
		outRMS->setControllableFeedbackOnly(true);
	}

}

Node::~Node()
{
}

void Node::clearItem()
{
	BaseItem::clearItem();
	if (nodeGraphPtr != nullptr && graph != nullptr) graph->removeNode(nodeGraphPtr.get());
	masterReference.clear();
}

void Node::init(AudioProcessorGraph* _graph)
{
	jassert(graph == nullptr);
	graph = _graph;

	processor->suspendProcessing(true);

	nodeGraphPtr = graph->addNode(std::unique_ptr<NodeAudioProcessor>(processor), AudioProcessorGraph::NodeID(nodeGraphID));

	initInternal();

	if (numAudioInputs != nullptr) autoSetNumAudioInputs();
	else updateAudioInputs(false);

	if (numAudioOutputs != nullptr) autoSetNumAudioOutputs();
	else updateAudioOutputs(false);

	updatePlayConfig();

	processor->suspendProcessing(false);
}

void Node::onContainerParameterChangedInternal(Parameter* p)
{
	if (p == enabled)
	{
		if (!enabled->boolValue()) for (auto& c : outAudioConnections) c->activityLevel = 0;
	}
	else if (p == numAudioInputs)
	{
		autoSetNumAudioInputs();
	}
	else if (p == numAudioOutputs)
	{
		autoSetNumAudioOutputs();
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
	updateAudioInputs();
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
	updateAudioOutputs();
	nodeListeners.call(&NodeListener::audioOutputsChanged, this);
	nodeNotifier.addMessage(new NodeEvent(NodeEvent::OUTPUTS_CHANGED, this));
}

void Node::autoSetNumAudioInputs()
{
	setAudioInputs(numAudioInputs->intValue());
}

void Node::autoSetNumAudioOutputs()
{
	setAudioOutputs(numAudioInputs->intValue());
}
void Node::updateAudioInputs(bool updateConfig)
{
	bool shouldResume = !processor->isSuspended();
	processor->suspendProcessing(true);
	updateAudioInputsInternal();
	if (updateConfig) updatePlayConfig();
	if (shouldResume) processor->suspendProcessing(false);
}

void Node::updateAudioOutputs(bool updateConfig)
{
	bool shouldResume = !processor->isSuspended();
	processor->suspendProcessing(true);
	updateAudioOutputsInternal();
	if (updateConfig)  updatePlayConfig();
	if (shouldResume) processor->suspendProcessing(false);
}

void Node::addInConnection(NodeConnection* c)
{
	processor->suspendProcessing(true);
	if (c->connectionType == NodeConnection::AUDIO) inAudioConnections.addIfNotAlreadyThere((NodeAudioConnection*)c);
	else if (c->connectionType == NodeConnection::MIDI) inMidiConnections.addIfNotAlreadyThere((NodeMIDIConnection*)c);
	processor->suspendProcessing(false);
}

void Node::removeInConnection(NodeConnection* c)
{
	processor->suspendProcessing(true);
	if (c->connectionType == NodeConnection::AUDIO) inAudioConnections.removeAllInstancesOf((NodeAudioConnection*)c);
	else if (c->connectionType == NodeConnection::MIDI) inMidiConnections.removeAllInstancesOf((NodeMIDIConnection*)c);
	processor->suspendProcessing(false);
}

void Node::addOutConnection(NodeConnection* c)
{
	processor->suspendProcessing(true);
	if (c->connectionType == NodeConnection::AUDIO) outAudioConnections.addIfNotAlreadyThere((NodeAudioConnection*)c);
	else if (c->connectionType == NodeConnection::MIDI) outMidiConnections.addIfNotAlreadyThere((NodeMIDIConnection*)c);
	processor->suspendProcessing(false);
}

void Node::removeOutConnection(NodeConnection* c)
{
	processor->suspendProcessing(true);
	if (c->connectionType == NodeConnection::AUDIO) outAudioConnections.removeAllInstancesOf((NodeAudioConnection*)c);
	else if (c->connectionType == NodeConnection::MIDI) outMidiConnections.removeAllInstancesOf((NodeMIDIConnection*)c);
	processor->suspendProcessing(false);
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

void Node::updatePlayConfig(bool notify)
{
	if (graph == nullptr)
	{
		DBG("Update play config, graph is null !");
		return;
	}

	bool shouldResume = !processor->isSuspended();
	processor->suspendProcessing(true);

	updatePlayConfigInternal();
	if (notify && !isCurrentlyLoadingData) nodeListeners.call(&NodeListener::nodePlayConfigUpdated, this);

	if (shouldResume) processor->suspendProcessing(false);
}

void Node::updatePlayConfigInternal()
{
	processor->setPlayConfigDetails(audioInputNames.size(), audioOutputNames.size(), graph->getSampleRate(), graph->getBlockSize());
}

void Node::receiveMIDIFromInput(Node* n, MidiBuffer& inputBuffer)
{
	inMidiBuffer.addEvents(inputBuffer, 0, processor->getBlockSize(), 0);
}



void Node::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
	if (!enabled->boolValue())
	{
		processBlockBypassed(buffer, midiMessages);
		return;
	}

	int numInputs = getNumAudioInputs();
	int numOutputs = getNumAudioOutputs();

	if (buffer.getNumChannels() < jmax(numInputs, numOutputs))
	{
		LOGWARNING("Not the same number of channels ! " << buffer.getNumChannels() << " < > " << jmax(numInputs, numOutputs));
		return;
	}

	processBlockInternal(buffer, inMidiBuffer);

	if (outGain != nullptr)
	{
		buffer.applyGain(outGain->floatValue());
	}

	float rms = 0;

	connectionsActivityLevels.fill(0);
	int numOutConnections = outAudioConnections.size();

	for (int i = 0; i < buffer.getNumChannels(); i++)
	{
		float channelRMS = buffer.getRMSLevel(i, 0, buffer.getNumSamples());
		rms = jmax(rms, channelRMS);

		for (int c = 0; c < numOutConnections; c++)
		{
			for (auto& cm : outAudioConnections[c]->channelMap)
			{
				if (cm.sourceChannel == i) connectionsActivityLevels.set(c, jmax(connectionsActivityLevels[c], channelRMS));
			}
		}
	}

	for (int i = 0; i < outAudioConnections.size(); i++)
	{
		float curLevel = outAudioConnections[i]->activityLevel;
		float level = connectionsActivityLevels[i];
		outAudioConnections[i]->activityLevel = curLevel + (level - curLevel) * .2f;
	}

	if (outRMS != nullptr)
	{
		float curVal = outRMS->floatValue();
		float targetVal = outRMS->getLerpValueTo(rms, rms > curVal ? .8f : .2f);
		outRMS->setValue(targetVal);
	}
}

BaseNodeViewUI* Node::createViewUI()
{
	return new BaseNodeViewUI(this);
}
