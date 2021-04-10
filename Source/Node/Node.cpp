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

Node::Node(StringRef name, var params, bool hasAudioInput, bool hasAudioOutput, bool userCanSetIO, bool useOutControl) :
	BaseItem(name, true),
	graph(nullptr),
	nodeGraphID(AudioManager::getInstance()->getNewGraphID()),
	hasAudioInput(hasAudioInput),
	hasAudioOutput(hasAudioOutput),
	hasMIDIInput(false),
	hasMIDIOutput(false),
	numAudioInputs(nullptr),
	numAudioOutputs(nullptr),
	viewCC("View"),
	showOutControl(nullptr),
	nodeNotifier(5)
{
	processor = new NodeAudioProcessor(this);
	processor->setPlayHead(Transport::getInstance());

	editorIsCollapsed = true;

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
		outControl.reset(new VolumeControl("Out", true));
		addChildControllableContainer(outControl.get());

		showOutControl = viewCC.addBoolParameter("Show Out Control", "Shows the Gain, RMS and Active on the right side in the view", true);
	}

	addChildControllableContainer(&viewCC);

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

	ScopedSuspender sp(processor);

	nodeGraphPtr = graph->addNode(std::unique_ptr<NodeAudioProcessor>(processor), AudioProcessorGraph::NodeID(nodeGraphID));

	initInternal();

	if (numAudioInputs != nullptr) autoSetNumAudioInputs();
	else updateAudioInputs(false);

	if (numAudioOutputs != nullptr) autoSetNumAudioOutputs();
	else updateAudioOutputs(false);

	updatePlayConfig();

}

void Node::onContainerParameterChangedInternal(Parameter* p)
{
	if (p == enabled)
	{
		if (!enabled->boolValue())
		{
			for (auto& c : outAudioConnections) c->activityLevel = 0;
			if (outControl != nullptr) outControl->rms->setValue(0);
		}
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

void Node::onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c)
{
	if (cc == &viewCC)
	{
		if(!isCurrentlyLoadingData) nodeNotifier.addMessage(new NodeEvent(NodeEvent::VIEW_FILTER_UPDATED, this));
	}
}

void Node::setAudioInputs(const int& numInputs, bool updateConfig)
{
	StringArray s;
	for (int i = 0; i < numInputs; i++) s.add("Input " + String(i + 1));
	setAudioInputs(s, updateConfig);
}

void Node::setAudioInputs(const StringArray& inputNames, bool updateConfig)
{
	if (audioInputNames == inputNames) return;
	ScopedSuspender sp(processor);

	audioInputNames = inputNames;
	updateAudioInputs(updateConfig);
	nodeListeners.call(&NodeListener::audioInputsChanged, this);
	nodeNotifier.addMessage(new NodeEvent(NodeEvent::INPUTS_CHANGED, this));
	processor->suspendProcessing(true);
}

void Node::setAudioOutputs(const int& numOutputs, bool updateConfig)
{
	StringArray s;
	for (int i = 0; i < numOutputs; i++) s.add("Output " + String(i + 1));
	setAudioOutputs(s, updateConfig);
}

void Node::setAudioOutputs(const StringArray& outputNames, bool updateConfig)
{
	if (audioOutputNames == outputNames) return;
	ScopedSuspender sp(processor);

	audioOutputNames = outputNames;
	updateAudioOutputs(updateConfig);
	nodeListeners.call(&NodeListener::audioOutputsChanged, this);
	nodeNotifier.addMessage(new NodeEvent(NodeEvent::OUTPUTS_CHANGED, this));
}

void Node::autoSetNumAudioInputs()
{
	setAudioInputs(numAudioInputs->intValue());
}

void Node::autoSetNumAudioOutputs()
{
	setAudioOutputs(numAudioOutputs->intValue());
}
void Node::updateAudioInputs(bool updateConfig)
{
	ScopedSuspender sp(processor);
	updateAudioInputsInternal();
	if (updateConfig) updatePlayConfig();
}

void Node::updateAudioOutputs(bool updateConfig)
{
	ScopedSuspender sp(processor);
	updateAudioOutputsInternal();
	if (updateConfig)  updatePlayConfig();
}

void Node::addInConnection(NodeConnection* c)
{
	ScopedSuspender sp(processor);
	if (c->connectionType == NodeConnection::AUDIO) inAudioConnections.addIfNotAlreadyThere((NodeAudioConnection*)c);
	else if (c->connectionType == NodeConnection::MIDI) inMidiConnections.addIfNotAlreadyThere((NodeMIDIConnection*)c);
}

void Node::removeInConnection(NodeConnection* c)
{
	ScopedSuspender sp(processor);
	if (c->connectionType == NodeConnection::AUDIO) inAudioConnections.removeAllInstancesOf((NodeAudioConnection*)c);
	else if (c->connectionType == NodeConnection::MIDI) inMidiConnections.removeAllInstancesOf((NodeMIDIConnection*)c);
}

void Node::addOutConnection(NodeConnection* c)
{
	ScopedSuspender sp(processor);
	if (c->connectionType == NodeConnection::AUDIO) outAudioConnections.addIfNotAlreadyThere((NodeAudioConnection*)c);
	else if (c->connectionType == NodeConnection::MIDI) outMidiConnections.addIfNotAlreadyThere((NodeMIDIConnection*)c);
}

void Node::removeOutConnection(NodeConnection* c)
{
	ScopedSuspender sp(processor);
	if (c->connectionType == NodeConnection::AUDIO) outAudioConnections.removeAllInstancesOf((NodeAudioConnection*)c);
	else if (c->connectionType == NodeConnection::MIDI) outMidiConnections.removeAllInstancesOf((NodeMIDIConnection*)c);
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

	ScopedSuspender sp(processor);

	updatePlayConfigInternal();
	if (notify && !isCurrentlyLoadingData) nodeListeners.call(&NodeListener::nodePlayConfigUpdated, this);
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

	if (processor->isSuspended())
	{
		LOGWARNING("Processor should be suspended, should not be here...");
		return;
	}

	ScopedLock sl(processor->getCallbackLock());

	int numInputs = getNumAudioInputs();
	int numOutputs = getNumAudioOutputs();

	if (buffer.getNumChannels() < jmax(numInputs, numOutputs))
	{
		if (Engine::mainEngine->isLoadingFile)
		{
			LOGWARNING("Should not be here");
		}
		LOGWARNING("Not the same number of channels ! " << buffer.getNumChannels() << " < > " << jmax(numInputs, numOutputs));
		return;
	}

	if (!enabled->boolValue())
	{
		processBlockBypassed(buffer, midiMessages);
		if (outControl != nullptr) outControl->updateRMS(buffer);
	}
	else
	{
		processBlockInternal(buffer, inMidiBuffer);
		if (outControl != nullptr) outControl->applyGain(buffer);
	}

	//float rms = 0;
	connectionsActivityLevels.fill(0);
	int numOutConnections = outAudioConnections.size();

	for (int i = 0; i < buffer.getNumChannels(); i++)
	{
		float channelRMS = buffer.getRMSLevel(i, 0, buffer.getNumSamples());
		jassert(!isnan(channelRMS) && !isinf(channelRMS));

		for (int c = 0; c < numOutConnections; c++)
		{
			for (auto& cm : outAudioConnections[c]->channelMap)
			{
				if (cm.sourceChannel == i) connectionsActivityLevels.set(c, jmax(connectionsActivityLevels[c], channelRMS));
			}
		}

		//rms = jmax(rms, channelRMS);
	}

	for (int i = 0; i < outAudioConnections.size(); i++)
	{
		float curLevel = outAudioConnections[i]->activityLevel;
		float level = connectionsActivityLevels[i];
		if (isnan(curLevel) || isinf(curLevel)) curLevel = level;
		outAudioConnections[i]->activityLevel = curLevel + (level - curLevel) * .2f;
	}

	/*
	if (outControl != nullptr && enabled->boolValue())
	{
		float curVal = outControl->rms->floatValue();
		float targetVal = outControl->rms->getLerpValueTo(rms, rms > curVal ? .8f : .2f);
		outControl->rms->setValue(targetVal);
	}
	*/
}

void Node::processBlockBypassed(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) 
{
	for (int i = getNumAudioInputs(); i < getNumAudioOutputs(); i++) buffer.clear(i, 0, buffer.getNumSamples());
}

var Node::getJSONData()
{
	var data = BaseItem::getJSONData();
	if (!saveAndLoadRecursiveData)
	{
		if (outControl != nullptr) data.getDynamicObject()->setProperty("out", outControl->getJSONData());
		data.getDynamicObject()->setProperty("view", viewCC.getJSONData());
	}
	return data;
}

void Node::loadJSONDataItemInternal(var data)
{
	if (!saveAndLoadRecursiveData)
	{
		if(outControl != nullptr) outControl->loadJSONData(data.getProperty("out", var()));
		viewCC.loadJSONData(data.getProperty("view", var()));
	}
}

BaseNodeViewUI* Node::createViewUI()
{
	return new BaseNodeViewUI(this);
}

NodeAudioProcessor::Suspender::Suspender(NodeAudioProcessor* proc) : proc(proc)
{
	proc->suspend();
}

NodeAudioProcessor::Suspender::~Suspender() {
	proc->resume();
}

void NodeAudioProcessor::suspend()
{
	suspendCount++;
	if (suspendCount == 1) suspendProcessing(true);
}


void NodeAudioProcessor::resume()
{
	suspendCount--;
	jassert(suspendCount >= 0);
	if (suspendCount == 0) suspendProcessing(false);
}

VolumeControl::VolumeControl(const String& name, bool hasRMS) :
	ControllableContainer(name),
	prevGain(1),
	rms(nullptr),
	rmsSampleCount(0),
	rmsMax(0)
{
	gain = new DecibelFloatParameter("Gain", "Gain for this");
	addParameter(gain);
	active = addBoolParameter("Active", "Fast way to mute this", true);

	if (hasRMS)
	{
		rms = addFloatParameter("RMS", "RMS for this", 0, 0, 1);
		rms->setControllableFeedbackOnly(true);
		rms->hideInRemoteControl = true; //hide by default
		rms->defaultHideInRemoteControl = true; //hide by default
	}
}

VolumeControl::~VolumeControl()
{
}

float VolumeControl::getGain()
{
	if (active == nullptr || gain == nullptr) return 0;
	return active->boolValue() ? gain->floatValue() : 0;
}

void VolumeControl::resetGainAndActive()
{
	gain->resetValue();
	active->resetValue();
}

void VolumeControl::applyGain(AudioSampleBuffer& buffer)
{
	float g = getGain();
	buffer.applyGainRamp(0, buffer.getNumSamples(), prevGain, g);
	prevGain = g;

	updateRMS(buffer);
}

void VolumeControl::applyGain(int channel, AudioSampleBuffer& buffer)
{
	float g = getGain();
	buffer.applyGainRamp(channel, 0, buffer.getNumSamples(), prevGain, g);
	prevGain = g;

	updateRMS(buffer, channel);
}

void VolumeControl::updateRMS(AudioSampleBuffer& buffer, int channel, int startSample, int numSamples)
{
	if (numSamples == -1) numSamples = buffer.getNumSamples();

	if (rms != nullptr)
	{
		if(channel >= 0) rmsMax = jmax(buffer.getRMSLevel(channel, startSample, numSamples), rmsMax);
		else
		{
			for (int i = 0; i < buffer.getNumChannels(); i++) rmsMax = jmax(buffer.getRMSLevel(i, startSample, numSamples), rmsMax);
		}

		rmsSampleCount += buffer.getNumSamples();
		if (rmsSampleCount > 4000) //~10fps @44100Hz
		{
			rms->setValue(rms->getLerpValueTo(rmsMax, rmsMax > rms->floatValue() ? .8f : .3f));
			rmsSampleCount = 0;
			rmsMax = 0;
		}
	}
}

DecibelFloatParameter::DecibelFloatParameter(const String& niceName, const String& description) :
	FloatParameter(niceName, description, Decibels::decibelsToGain(0), 0, Decibels::decibelsToGain(6.0f))
{
	
}

DecibelFloatParameter::~DecibelFloatParameter()
{
}

ControllableUI* DecibelFloatParameter::createDefaultUI()
{
	return new DecibelSliderUI(this);
}
