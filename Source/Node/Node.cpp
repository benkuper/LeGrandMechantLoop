/*
  ==============================================================================

	Node.cpp
	Created: 15 Nov 2020 8:40:03am
	Author:  bkupe

  ==============================================================================
*/

#include "Node/NodeIncludes.h"
#include "Interface/InterfaceIncludes.h"

Node::Node(StringRef name, var params, bool hasAudioInput, bool hasAudioOutput, bool userCanSetIO, bool useOutControl, bool canHaveMidiDeviceIn, bool canHaveMidiDeviceOut) :
	BaseItem(name, true),
	graph(nullptr),
	nodeGraphID(AudioManager::getInstance()->getNewGraphID()),
	hasAudioInput(hasAudioInput),
	hasAudioOutput(hasAudioOutput),
	hasMIDIInput(false),
	hasMIDIOutput(false),
	numAudioInputs(nullptr),
	numAudioOutputs(nullptr),
	midiInterfaceParam(nullptr),
	midiInterface(nullptr),
	//currentInDevice(nullptr),
	//currentOutDevice(nullptr),
	pedalSustain(nullptr),
	forceSustain(nullptr),
	viewCC("View"),
	showOutControl(nullptr),
	bypassAntiClickCount(anticlickBlocks),
	nodeNotifier(5)
{
	showWarningInUI = true;

	setHasCustomColor(true);
	itemColor->setDefaultValue(Colours::darkgrey);

	processor = new NodeAudioProcessor(this);
	processor->setPlayHead(Transport::getInstance());

	hideInEditor = true;
	editorIsCollapsed = true;

	isNodePlaying = addBoolParameter("Is Node Playing", "This is a feedback to know if a node has playing content. Used by the global time to automatically stop if no content is playing", false);
	isNodePlaying->setControllableFeedbackOnly(true);
	//isNodePlaying->hideInEditor = true;
	isNodePlaying->hideInRemoteControl = true;
	isNodePlaying->defaultHideInRemoteControl = true;
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

	viewCC.hideInRemoteControl = true;
	viewCC.defaultHideInRemoteControl = true;
	addChildControllableContainer(&viewCC);

	if (canHaveMidiDeviceIn || canHaveMidiDeviceOut)
	{
		midiCC.reset(new ControllableContainer("MIDI"));
		midiCC->saveAndLoadRecursiveData = true;

		midiInterfaceParam = midiCC->addTargetParameter("MIDI Interface", "The interface to get midi messages from", InterfaceManager::getInstance());// canHaveMidiDeviceIn, canHaveMidiDeviceOut);
		midiInterfaceParam->targetType = TargetParameter::CONTAINER;
		midiInterfaceParam->maxDefaultSearchLevel = 0;

		pedalSustain = midiCC->addBoolParameter("Pedal Sustain", "If enabled, incoming CC64 midi messages from device will be treated as incoming pedal and this will be the feedback for its state", false);
		pedalSustain->canBeDisabledByUser = true;
		pedalSustain->setControllableFeedbackOnly(true);
		forceSustain = midiCC->addBoolParameter("Force Sustain", "If checked, this will force sustain manually independent of the pedal", false);
		forceNoteOffOnEnabled = midiCC->addBoolParameter("Force Note offs on enabled", "If checked, this will send all notes off when enabled state is changed", false);
		logIncomingMidi = midiCC->addBoolParameter("Log Incoming messages", "This will log incoming messages (only working on the device directly connected, not incoming connection from other nodes", false);
		channelFilterCC.reset(new ControllableContainer("Channel Filter"));
		midiChannels.ensureStorageAllocated(16);
		for (int i = 1; i <= 16; i++) midiChannels.set(i, channelFilterCC->addBoolParameter(String(i), "If checked, this node will accept messages from this channel", true));
		channelFilterCC->editorIsCollapsed = true;
		midiCC->addChildControllableContainer(channelFilterCC.get());

		addChildControllableContainer(midiCC.get());

		setMIDIIO(canHaveMidiDeviceIn, canHaveMidiDeviceOut);
	}

}

Node::~Node()
{
}

void Node::clearItem()
{
	BaseItem::clearItem();
	setMIDIInterface(nullptr);
	//setMIDIOutDevice(nullptr);
	if (nodeGraphPtr != nullptr && graph != nullptr) graph->removeNode(nodeGraphPtr.get());
	masterReference.clear();
}

void Node::init(AudioProcessorGraph* _graph)
{
	jassert(graph == nullptr);
	graph = _graph;

	ScopedSuspender sp(processor);


	initInternal();

	if (numAudioInputs != nullptr && numAudioInputs->enabled) autoSetNumAudioInputs();
	else updateAudioInputs(false);

	if (numAudioOutputs != nullptr && numAudioInputs->enabled) autoSetNumAudioOutputs();
	else updateAudioOutputs(false);

	updatePlayConfig();

	nodeGraphPtr = graph->addNode(std::unique_ptr<NodeAudioProcessor>(processor), AudioProcessorGraph::NodeID(nodeGraphID));

}

void Node::onContainerParameterChangedInternal(Parameter* p)
{
	if (p == enabled)
	{
		if (!enabled->boolValue())
		{
			for (auto& c : outAudioConnections) c->activityLevel = 0;
			if (outControl != nullptr) outControl->rms->setValue(0);
			if (processor->getSampleRate() > 0) midiCollector.reset(processor->getSampleRate());
			updateSustainedNotes();
		}

		if (hasMIDIInput && forceNoteOffOnEnabled->boolValue())
		{
            if(processor->getSampleRate() > 0)
            {
                
                midiCollector.reset(processor->getSampleRate());
                for (int i = 1; i <= 16; i++)
                {
                    MidiMessage m(MidiMessage::allNotesOff(i), 1);
                    midiCollector.addMessageToQueue(m);
                }
            }
		}
	}
	else if (p == numAudioInputs)
	{
		if (numAudioInputs->enabled) autoSetNumAudioInputs();
	}
	else if (p == numAudioOutputs)
	{
		if (numAudioOutputs->enabled) autoSetNumAudioOutputs();
	}
}

void Node::onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c)
{
	if (cc == &viewCC)
	{
		if (!isCurrentlyLoadingData) nodeNotifier.addMessage(new NodeEvent(NodeEvent::VIEW_FILTER_UPDATED, this));
	}
	else if (c == midiInterfaceParam)
	{
		setMIDIInterface((MIDIInterface*)midiInterfaceParam->targetContainer.get());
		//setMIDIOutDevice(midiInterfaceParam->outputDevice);
	}
	else if (c == pedalSustain || c == forceSustain)
	{
		if (!forceSustain->boolValue()) updateSustainedNotes();
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

void Node::setMIDIInterface(MIDIInterface* i)
{
	if (midiInterface == i) return;
	if (midiInterface != nullptr)
	{
		midiInterface->removeMIDIInterfaceListener(this);
	}

	midiInterface = i;

	if (midiInterface != nullptr)
	{
		midiInterface->addMIDIInterfaceListener(this);
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
    
    if(graph->getSampleRate() > 0) midiCollector.reset(graph->getSampleRate());
}

void Node::midiMessageReceived(MIDIInterface* i, const MidiMessage& m)
{
	if (!enabled->boolValue()) return;
	if (m.getChannel() > 0 && !midiChannels[m.getChannel() - 1]->boolValue()) return;

	if (logIncomingMidi->boolValue())
	{
		NLOG(niceName, "Received MIDI : " << m.getDescription());
	}

	bool addToQueue = true;

	if (m.isSustainPedalOn() || m.isSustainPedalOff())
	{
		if (pedalSustain->enabled) pedalSustain->setValue(m.isSustainPedalOn());
	}

	if (m.isNoteOff())
	{
		if (pedalSustain->boolValue() || forceSustain->boolValue())
		{
			sustainedNotes.set(m.getNoteNumber(), m.getChannel());
			addToQueue = false;
		}
		else sustainedNotes.remove(m.getNoteNumber());
	}
	else if (m.isNoteOn())
	{
		sustainedNotes.remove(m.getNoteNumber());
	}

	if (addToQueue) midiCollector.addMessageToQueue(m);
}

void Node::updateSustainedNotes()
{
	if (!enabled->boolValue() || (!pedalSustain->boolValue() && !forceSustain->boolValue()))
	{
		HashMap<int, int>::Iterator it(sustainedNotes);
		while (it.next()) midiCollector.addMessageToQueue(MidiMessage::noteOff(it.getValue(), it.getKey()).withTimeStamp(Time::currentTimeMillis()));
		sustainedNotes.clear();
	}
}


void Node::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock)
{
	if (sampleRate != 0)
    {
        midiCollector.reset(sampleRate);
    }
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
    
	if (buffer.getNumChannels() != jmax(numInputs, numOutputs))
	{
		jassert(!Engine::mainEngine->isLoadingFile);

        if(!channelMismatch)
        {
            setWarningMessage("Not the same number of channels ! " + String(buffer.getNumChannels()) + " < > " + String(jmax(numInputs, numOutputs)), "Channel Mismatch");
        }
        channelMismatch = true;
        return;
	}else if(channelMismatch)
    {
        channelMismatch = false;
        clearWarning("Channel Mismatch");
    }
    
	//MIDI
    if(hasMIDIInput) midiCollector.removeNextBlockOfMessages(midiMessages, buffer.getNumSamples());

	bool isEnabled = enabled->boolValue();
	bool antiClickFinished = bypassAntiClickCount == (isEnabled ? anticlickBlocks : 0);
	if (antiClickFinished)
	{

		if (isEnabled)
		{
			processBlockInternal(buffer, midiMessages);
			if (outControl != nullptr) outControl->applyGain(buffer);
		}
		else
		{
			processBlockBypassed(buffer, midiMessages);
			if (outControl != nullptr) outControl->updateRMS(buffer);
		}
	}
	else
	{
		AudioSampleBuffer b1;
		b1.makeCopyOf(buffer);
		AudioSampleBuffer b2;
		b2.makeCopyOf(buffer);
		processBlockInternal(b1, midiMessages);
		processBlockBypassed(b2, midiMessages);
		buffer.clear();

		float curVal = bypassAntiClickCount * 1.0f / anticlickBlocks;
		bypassAntiClickCount += isEnabled ? 1 : -1;
		float nextVal = bypassAntiClickCount * 1.0f / anticlickBlocks;

		if (!antiClickFinished && bypassAntiClickCount == 0)
		{
			bypassInternal();
		}

		for (int i = 0; i < buffer.getNumChannels(); i++)
		{
			buffer.addFromWithRamp(i, 0, b1.getReadPointer(i), buffer.getNumSamples(), curVal, nextVal);
			buffer.addFromWithRamp(i, 0, b2.getReadPointer(i), buffer.getNumSamples(), 1 - curVal, 1 - nextVal);
		}
	}

	connectionsActivityLevels.fill(0);
	int numOutConnections = outAudioConnections.size();

	for (int i = 0; i < buffer.getNumChannels(); i++)
	{
		float channelRMS = buffer.getRMSLevel(i, 0, buffer.getNumSamples());
		if (isinf(channelRMS) || isnan(channelRMS)) channelRMS = 0;

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
		if (isnan(curLevel) || isinf(curLevel)) curLevel = level;
		outAudioConnections[i]->activityLevel = curLevel + (level - curLevel) * .2f;
	}

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
		if (midiCC != nullptr) data.getDynamicObject()->setProperty("midi", midiCC->getJSONData());
	}
	return data;
}

void Node::loadJSONDataItemInternal(var data)
{
	if (!saveAndLoadRecursiveData)
	{
		if (outControl != nullptr) outControl->loadJSONData(data.getProperty("out", var()));
		viewCC.loadJSONData(data.getProperty("view", var()));
		if (midiCC != nullptr) midiCC->loadJSONData(data.getProperty("midi", var()));
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

void NodeAudioProcessor::numChannelsChanged()
{
	NLOG(node->niceName, "Channels changed : " << getTotalNumInputChannels() << ":" << getTotalNumOutputChannels());
}
