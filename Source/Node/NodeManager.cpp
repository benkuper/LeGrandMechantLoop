/*
  ==============================================================================

	NodeManager.cpp
	Created: 15 Nov 2020 8:39:59am
	Author:  bkupe

  ==============================================================================
*/

#include "Node/NodeIncludes.h"

juce_ImplementSingleton(RootNodeManager)

NodeManager::NodeManager(AudioProcessorGraph* graph,
	AudioProcessorGraph::NodeID audioInputNodeID, AudioProcessorGraph::NodeID audioOutputNodeID,
	AudioProcessorGraph::NodeID midiInputNodeID, AudioProcessorGraph::NodeID midiOutputNodeID) :
	Manager("Nodes"),
	graph(graph),
	cpuUsage(nullptr),
	audioInputNodeID(audioInputNodeID),
	audioOutputNodeID(audioOutputNodeID),
	midiInputNodeID(midiInputNodeID),
	midiOutputNodeID(midiOutputNodeID),
	looperControlCC("Looper Control"),
	presetsCC("Connection Presets"),
	customParametersCC("Custom Parameters")
{

	managerFactory = NodeFactory::getInstance();

	comparator.compareFunc = [](Node* n1, Node* n2) { return n1->niceName.compare(n2->niceName); };

	isPlaying = addBoolParameter("Is Node Playing", "This is a feedback to know if a node has playing content. Used by the global time to automatically stop if no content is playing", false);
	isPlaying->setControllableFeedbackOnly(true);
	isPlaying->hideInEditor = true;

	playAllLoopers = looperControlCC.addTrigger("Play All Loopers", "This will play all loopers");
	stopAllLoopers = looperControlCC.addTrigger("Stop All Loopers", "This will stop all loopers");
	clearAllLoopers = looperControlCC.addTrigger("Clear All Loopers", "This will clear all loopers");
	tmpMuteAllLoopers = looperControlCC.addTrigger("Temp Mute All Loopers", "This will temporary mute all loopers");
	clearLastManipTrack = looperControlCC.addTrigger("Clear Last Manip Track", "This will clear the last manipulated track, in any looper");
	currentLooperEnum = looperControlCC.addEnumParameter("Current Looper", "This is the current looper selected in the UI");
	recCurrentLooper = looperControlCC.addTrigger("Rec Current Looper", "This will record the current looper");
    clearCurrentLooper = looperControlCC.addTrigger("Clear Current Looper", "This will clear the current looper's last track");
    retroRecCurrentLooper = looperControlCC.addTrigger("Retro Rec Current Looper", "This will retro record the current looper");
	playAllCurrentLooper = looperControlCC.addTrigger("Play All Current Looper", "This will play all tracks of the current looper");
	stopAllCurrentLooper = looperControlCC.addTrigger("Stop All Current Looper", "This will stop all tracks of the current looper");
	clearAllCurrentLooper = looperControlCC.addTrigger("Clear All Current Looper", "This will clear all tracks of the current looper");
	addChildControllableContainer(&looperControlCC);

	addChildControllableContainer(&customParametersCC);
	customParametersCC.userCanAddControllables = true;

	connectionManager.reset(new NodeConnectionManager(this));
	connectionManager->hideInRemoteControl = true;
	connectionManager->defaultHideInRemoteControl = true;
	addChildControllableContainer(connectionManager.get());


	presetEnum = presetsCC.addEnumParameter("Presets", "Patch configurations");
	reloadPreset = presetsCC.addTrigger("Reload Preset", "Reload the current preset");
	addPreset = presetsCC.addTrigger("Add Preset", "Add a new preset");
	savePreset = presetsCC.addTrigger("Save Preset", "Save the current state as a preset");
	newPresetName = presetsCC.addStringParameter("New Preset Name", "Name for the new preset when triggering add preset", "New Preset");
	updatePresetName = presetsCC.addTrigger("Update Preset Name", "Update the name of the current preset");
	deletePreset = presetsCC.addTrigger("Delete Preset", "Delete the current preset");
	addChildControllableContainer(&presetsCC);


	presets = var(new DynamicObject());

	setHasGridOptions(true);

}


NodeManager::~NodeManager()
{
}


void NodeManager::clear()
{
	connectionManager->clear();
	Manager::clear();
}

void NodeManager::setAudioInputs(const int& numInputs)
{
	StringArray s;
	for (int i = 0; i < numInputs; i++) s.add("Input " + String(i + 1));
	setAudioInputs(s);
}

void NodeManager::setAudioInputs(const StringArray& inputNames)
{
	audioInputNames = inputNames;
	for (auto& n : audioInputNodes) updateAudioInputNode(n);

}

void NodeManager::setAudioOutputs(const int& numOutputs)
{
	StringArray s;
	for (int i = 0; i < numOutputs; i++) s.add("Output " + String(i + 1));
	setAudioOutputs(s);
}

void NodeManager::setAudioOutputs(const StringArray& outputNames)
{
	audioOutputNames = outputNames;
	for (auto& n : audioOutputNodes) updateAudioOutputNode(n);
}


void NodeManager::updateAudioInputNode(AudioInputNode* n)
{
	for (int i = 0; i < n->audioInputNames.size(); i++)
	{
		graph->removeConnection(AudioProcessorGraph::Connection({ audioInputNodeID, i }, { n->nodeGraphID, i }), AudioProcessorGraph::UpdateKind::async); //straight channel 
	}

	n->setAudioOutputs(audioInputNames); //inverse to get good connector names

	for (int i = 0; i < audioInputNames.size(); i++)
	{
		graph->addConnection(AudioProcessorGraph::Connection({ audioInputNodeID, i }, { n->nodeGraphID, i }), AudioProcessorGraph::UpdateKind::async); //straight 
	}
}

void NodeManager::updateAudioOutputNode(AudioOutputNode* n)
{
	for (int i = 0; i < n->audioOutputNames.size(); i++)
	{
		graph->removeConnection(AudioProcessorGraph::Connection({ n->nodeGraphID, i }, { audioOutputNodeID, i }), AudioProcessorGraph::UpdateKind::async); //straight channel 
	}

	n->setAudioInputs(audioOutputNames); //inverse to get good connector names

	for (int i = 0; i < audioOutputNames.size(); i++)
	{
		graph->addConnection(AudioProcessorGraph::Connection({ n->nodeGraphID, i }, { audioOutputNodeID, i }), AudioProcessorGraph::UpdateKind::async); //straight 
	}
}

void NodeManager::updateMIDIInputNode(MIDIInputNode* n)
{
	graph->addConnection(AudioProcessorGraph::Connection({ midiInputNodeID, AudioProcessorGraph::midiChannelIndex }, { n->nodeGraphID, AudioProcessorGraph::midiChannelIndex }), AudioProcessorGraph::UpdateKind::async); //straight 
}

void NodeManager::updateMIDIOutputNode(MIDIOutputNode* n)
{
	graph->addConnection(AudioProcessorGraph::Connection({ n->nodeGraphID, AudioProcessorGraph::midiChannelIndex }, { midiOutputNodeID, AudioProcessorGraph::midiChannelIndex }), AudioProcessorGraph::UpdateKind::async); //straight 
}

void NodeManager::addItemInternal(Node* n, var data)
{
	n->init(graph);

	bool isRoot = this == RootNodeManager::getInstance();

	if (AudioInputNode* in = dynamic_cast<AudioInputNode*>(n))
	{
		in->setIsRoot(isRoot);
		audioInputNodes.add(in);
		updateAudioInputNode(in);
	}
	else if (AudioOutputNode* on = dynamic_cast<AudioOutputNode*>(n))
	{
		on->setIsRoot(isRoot);
		audioOutputNodes.add(on);
		updateAudioOutputNode(on);
	}
	else if (MIDIInputNode* mio = dynamic_cast<MIDIInputNode*>(n))
	{
		midiInputNodes.add(mio);
		updateMIDIInputNode(mio);
	}
	else if (LooperNode* looper = dynamic_cast<LooperNode*>(n)) updateLooperList();
}

void NodeManager::removeItemInternal(Node* n)
{
	connectionManager->removeAllLinkedConnections(n);

	if (AudioInputNode* in = dynamic_cast<AudioInputNode*>(n))  audioInputNodes.removeAllInstancesOf(in);
	else if (AudioOutputNode* on = dynamic_cast<AudioOutputNode*>(n)) audioOutputNodes.removeAllInstancesOf(on);
	else if (MIDIInputNode* mio = dynamic_cast<MIDIInputNode*>(n)) midiInputNodes.removeAllInstancesOf(mio);
	else if (MIDIOutputNode* moo = dynamic_cast<MIDIOutputNode*>(n)) midiOutputNodes.removeAllInstancesOf(moo);
	else if (LooperNode* looper = dynamic_cast<LooperNode*>(n)) updateLooperList();
}

Array<UndoableAction*> NodeManager::getRemoveItemUndoableAction(Node* item)
{
	Array<UndoableAction*> result;
	Array<Node*> itemsToRemove;
	itemsToRemove.add(item);
	result.addArray(connectionManager->getRemoveAllLinkedConnectionsActions(itemsToRemove));
	result.addArray(Manager::getRemoveItemUndoableAction(item));
	return result;
}

Array<UndoableAction*> NodeManager::getRemoveItemsUndoableAction(Array<Node*> itemsToRemove)
{
	Array<UndoableAction*> result;
	result.addArray(connectionManager->getRemoveAllLinkedConnectionsActions(itemsToRemove));
	result.addArray(Manager::getRemoveItemsUndoableAction(itemsToRemove));
	return result;
}

void NodeManager::onControllableFeedbackUpdate(ControllableContainer* cc, Controllable* c)
{
	if (Node* n = c->getParentAs<Node>())
	{
		if (c == n->isNodePlaying)
		{
			isPlaying->setValue(hasPlayingNodes());
		}
	}
	else if (c == playAllLoopers)
	{
		Array<LooperNode*> loopers = getLoopers(true, true, true);
		for (auto& looper : loopers) looper->playAllTrigger->trigger();
	}
	else if (c == stopAllLoopers)
	{
		Array<LooperNode*> loopers = getLoopers(true, true, true);
		for (auto& looper : loopers) looper->stopAllTrigger->trigger();
			
	}
	else if (c == clearAllLoopers)
	{
		Array<LooperNode*> loopers = getLoopers(true, true, false);
		for (auto& looper : loopers)  looper->clearAllTrigger->trigger();
	}
	else if (c == tmpMuteAllLoopers)
	{
		Array<LooperNode*> loopers = getLoopers(true, true, true);
		for (auto& looper : loopers) looper->tmpMuteAllTrigger->trigger();
	}
	else if (c == clearLastManipTrack)
	{
		if (LooperTrack* t = LooperTrack::lastManipulatedTracks.getLast())
		{
			if (containsControllable(t->clearTrigger)) t->clearTrigger->trigger();
		}
		LooperTrack::lastManipulatedTracks.removeLast();
	}
	else if (c == currentLooperEnum)
	{

	}
	else if (c == recCurrentLooper)
	{
		if (LooperNode* looper = dynamic_cast<LooperNode*>(getItemWithName(currentLooperEnum->getValue())))
		{
			looper->recTrigger->trigger();
		}
	}else if (c == clearCurrentLooper)
    {
        if (LooperNode* looper = dynamic_cast<LooperNode*>(getItemWithName(currentLooperEnum->getValue())))
        {
            looper->clearCurrentTrigger->trigger();
        }
    }
	else if (c == retroRecCurrentLooper)
	{
		if (LooperNode* looper = dynamic_cast<LooperNode*>(getItemWithName(currentLooperEnum->getValue())))
		{
			looper->retroRecTrigger->trigger();
		}
	}
	else if (c == playAllCurrentLooper)
	{
		if (LooperNode* looper = dynamic_cast<LooperNode*>(getItemWithName(currentLooperEnum->getValue())))
		{
			looper->playAllTrigger->trigger();
		}
	}
	else if (c == stopAllCurrentLooper)
	{
		if (LooperNode* looper = dynamic_cast<LooperNode*>(getItemWithName(currentLooperEnum->getValue())))
		{
			looper->stopAllTrigger->trigger();
		}
	}
	else if (c == clearAllCurrentLooper)
	{
		if (LooperNode* looper = dynamic_cast<LooperNode*>(getItemWithName(currentLooperEnum->getValue())))
		{
			looper->clearAllTrigger->trigger();
		}
	}



	if (cc == &presetsCC)
	{
		if (c == presetEnum)
		{
			loadCurrentPreset();
		}
		else if (c == reloadPreset)
		{
			loadCurrentPreset();
		}
		else if (c == addPreset)
		{
			String s = newPresetName->stringValue();
			if (s.isEmpty()) s = "New preset";
			String uniqueName = s;
			int i = 1;
			while (presets.hasProperty(uniqueName))
			{
				uniqueName = s + " " + String(i);
				i++;
			}
			presets.getDynamicObject()->setProperty(uniqueName, connectionManager->getJSONData());
			updatePresetEnum();
			presetEnum->setValueWithKey(uniqueName);
		}
		else if (c == savePreset)
		{
			if (presetEnum->getValueKey().isNotEmpty())
			{
				presets.getDynamicObject()->setProperty(presetEnum->getValueKey(), connectionManager->getJSONData());
				presetEnum->setValueWithKey(presetEnum->getValueKey());
			}
		}
		else if (c == updatePresetName)
		{
			presets.getDynamicObject()->setProperty(newPresetName->stringValue(), presets.getProperty(presetEnum->getValueKey(), var()));
			presets.getDynamicObject()->removeProperty(presetEnum->getValueKey());
			updatePresetEnum();
			presetEnum->setValueWithKey(newPresetName->stringValue());
		}
		else if (c == deletePreset)
		{
			//delete preset
			presets.getDynamicObject()->removeProperty(presetEnum->getValueKey());
			updatePresetEnum();
		}
	}
}

void NodeManager::updateLooperList()
{
	Array<EnumParameter::EnumValue> looperOptions;
	Array<LooperNode*> loopers = getItemsWithType<LooperNode>();
	for (auto& i : loopers) looperOptions.add(EnumParameter::EnumValue(i->niceName, i->shortName));
	currentLooperEnum->setOptions(looperOptions);
}

void NodeManager::setCurrentLooper(LooperNode* n)
{
	currentLooperEnum->setValueWithKey(n->niceName);
}

bool NodeManager::hasPlayingNodes()
{
	for (auto& i : items)
	{
		if (LooperNode* looper = dynamic_cast<LooperNode*>(i))
		{
			if (looper->excludeFromTransportCheck->boolValue()) continue;
		}

		if (i->isNodePlaying->boolValue()) return true;
	}
	return false;
}

void NodeManager::updatePresetEnum()
{
	if (isCurrentlyLoadingData) return;

	if (presets.isVoid()) presets = var(new DynamicObject()); //safety

	Array<EnumParameter::EnumValue> options;
	options.add(EnumParameter::EnumValue("None", var()));


	NamedValueSet& nvSet = presets.getDynamicObject()->getProperties();
	for (auto& nv : nvSet) options.add(EnumParameter::EnumValue(nv.name.toString(), nv.name.toString()));
	presetEnum->setOptions(options);
}

void NodeManager::loadCurrentPreset()
{
	if (isCurrentlyLoadingData) return;

    var pData = presetEnum->getValue();
    var data = pData.isVoid()? var(new DynamicObject()) : presets.getProperty(pData.toString(), var());
    connectionManager->loadJSONData(data);
}

bool NodeManager::handleRemoteControlData(const OSCMessage& msg, const String& clientId)
{
	String s = msg.getAddressPattern().toString().substring(getControlAddress().length());
	LOG(s);

	if (s == "/getSetup")
	{
	//	OSCMessage im("/inSlots");
	//	OSCMessage om("/outSlots");
	//	for (auto& n : items)
	//	{
	//		for (int i = 0; i < n->audioInputNames.size(); i++) im.addString(n->niceName + ":" + n->audioInputNames[i]);
	//		for (int i = 0; i < n->audioOutputNames.size(); i++) om.addString(n->niceName + ":" + n->audioOutputNames[i]);
	//	}
	//	OSCRemoteControl::getInstance()->sendManualFeedback(im);
	//	OSCRemoteControl::getInstance()->sendManualFeedback(om);


	//	OSCMessage am("/audioConnections");
	//	for (auto& c : connectionManager->items)
	//	{
	//		if (c->connectionType != NodeConnection::ConnectionType::AUDIO) continue;
	//		NodeAudioConnection* ac = dynamic_cast<NodeAudioConnection*>(c);
	//		am.addString(ac->sourceNode->niceName);
	//		am.addString(ac->destNode->niceName);
	//		am.addInt32(ac->channelMap.size());
	//		for (auto& m : ac->channelMap)
	//		{
	//			String sourceOutputName = ac->sourceNode->audioOutputNames[m.sourceChannel];
	//			String destInputName = ac->destNode->audioInputNames[m.destChannel];
	//			am.addString(sourceOutputName + ":" + destInputName);
	//		}
	//	}
	//	OSCRemoteControl::getInstance()->sendManualFeedback(am);
	}
	else if (s == "/connectAudio")
	{
		Node* n1 = getItemWithName(OSCHelpers::getStringArg(msg[0]), true);
		Node* n2 = getItemWithName(OSCHelpers::getStringArg(msg[1]), true);
		String outputName = OSCHelpers::getStringArg(msg[2]);
		String inputName = OSCHelpers::getStringArg(msg[3]);

		if (n1 != nullptr && n2 != nullptr)
		{
			connectionManager->addConnection(n1, n2, NodeConnection::AUDIO);
			NodeAudioConnection* ac = dynamic_cast<NodeAudioConnection*>(connectionManager->getConnectionForSourceAndDest(n1, n2, NodeConnection::AUDIO));
			if (ac != nullptr)
			{
				int sourceChannel = n1->audioOutputNames.indexOf(outputName);
				int destChannel = n2->audioInputNames.indexOf(inputName);

				ac->connectChannels(sourceChannel, destChannel);
			}

		}
	}
	return false;
}

void NodeManager::sendConnectionsToRemoteControl()
{
	if (!OSCRemoteControl::getInstance()->manualSendCC.enabled->boolValue()) return;

}

Array<LooperNode*> NodeManager::getLoopers(bool recursive, bool checkControl, bool checkTransport)
{
	Array<LooperNode*> result;
	for (auto& n : items)
	{
		if (LooperNode* looper = dynamic_cast<LooperNode*>(n))
		{
			if (!looper->enabled->boolValue()) continue;
			if (checkControl && looper->excludeFromGlobalControl->boolValue()) continue;
			if (checkTransport && looper->excludeFromTransportCheck->boolValue()) continue;
			result.add(looper);
		}
		else if (ContainerNode* cNode = dynamic_cast<ContainerNode*>(n))
		{
			if (!cNode->enabled->boolValue()) continue;
			if (recursive)
			{
				result.addArray(cNode->nodeManager->getLoopers(true, checkControl, checkTransport));
			}
		}
	}

	return result;
}


var NodeManager::getJSONData(bool includeNonOverriden)
{
	var data = Manager::getJSONData(includeNonOverriden);
	data.getDynamicObject()->setProperty(looperControlCC.shortName, looperControlCC.getJSONData());
	data.getDynamicObject()->setProperty(connectionManager->shortName, connectionManager->getJSONData());
	data.getDynamicObject()->setProperty(presetsCC.shortName, presetsCC.getJSONData());
	data.getDynamicObject()->setProperty(customParametersCC.shortName, customParametersCC.getJSONData());
	data.getDynamicObject()->setProperty("connectionPresetData", presets);
	return data;
}

void NodeManager::loadJSONDataManagerInternal(var data)
{
	Manager::loadJSONDataManagerInternal(data);
	looperControlCC.loadJSONData(data.getProperty(looperControlCC.shortName, var()));
	presets = data.getProperty("connectionPresetData", var(new DynamicObject()));
	presetsCC.loadJSONData(data.getProperty(presetsCC.shortName, var()));
	connectionManager->loadJSONData(data.getProperty(connectionManager->shortName, var()));
	customParametersCC.loadJSONData(data.getProperty(customParametersCC.shortName, var()));
}

void NodeManager::afterLoadJSONDataInternal()
{
	Manager::afterLoadJSONDataInternal();
	updatePresetEnum();
	updateLooperList();

}


//ROOT

RootNodeManager::RootNodeManager() :
	NodeManager(&AudioManager::getInstance()->graph,
		AudioProcessorGraph::NodeID(AUDIO_GRAPH_INPUT_ID),
		AudioProcessorGraph::NodeID(AUDIO_GRAPH_OUTPUT_ID),
		AudioProcessorGraph::NodeID(MIDI_GRAPH_INPUT_ID),
		AudioProcessorGraph::NodeID(MIDI_GRAPH_OUTPUT_ID))
{
	AudioManager::getInstance()->addAudioManagerListener(this);

	setAudioInputs(AudioManager::getInstance()->getInputChannelNames());
	setAudioOutputs(AudioManager::getInstance()->getOutputChannelNames());

	Engine::mainEngine->addEngineListener(this);
}

RootNodeManager::~RootNodeManager()
{
	Engine::mainEngine->removeEngineListener(this);
	AudioManager::getInstance()->removeAudioManagerListener(this);
}

void RootNodeManager::clear()
{
	NodeManager::clear();
	customParametersCC.clear();
}

void RootNodeManager::audioSetupChanged()
{
	setAudioInputs(AudioManager::getInstance()->getInputChannelNames());
	setAudioOutputs(AudioManager::getInstance()->getOutputChannelNames());
}

void RootNodeManager::onContainerParameterChanged(Parameter* p)
{
	NodeManager::onContainerParameterChanged(p);

	if (p == isPlaying)
	{
		if (!isPlaying->boolValue())
		{
            Transport::AutoStopBehaviour b = Transport::getInstance()->autoStopBehaviour->getValueDataAsEnum<Transport::AutoStopBehaviour>();
            
            if(b == Transport::LAST_NODE || (b == Transport::LAST_NODE_NOPREPLAY && Transport::getInstance()->transportWasStartedFromNode))
            {
                Transport::getInstance()->stopTrigger->trigger();
            }
		}
	}
}

void RootNodeManager::addItemInternal(Node* n, var data)
{
	NodeManager::addItemInternal(n, data);
	n->addNodeListener(this);
	if (!isCurrentlyLoadingData) AudioManager::getInstance()->updateGraph();
}

void RootNodeManager::removeItemInternal(Node* n)
{
	NodeManager::removeItemInternal(n);
	n->removeNodeListener(this);
}

void RootNodeManager::nodePlayConfigUpdated(Node* n)
{
	if (!isCurrentlyLoadingData) AudioManager::getInstance()->updateGraph();
}

void RootNodeManager::endLoadFile()
{
	for (auto& i : items) i->updatePlayConfig(false);
	AudioManager::getInstance()->updateGraph();
}
