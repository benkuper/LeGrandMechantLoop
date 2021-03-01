/*
  ==============================================================================

	NodeConnection.cpp
	Created: 16 Nov 2020 10:00:05am
	Author:  bkupe

  ==============================================================================
*/

#include "NodeConnection.h"
#include "Engine/AudioManager.h"
#include "Node/NodeManager.h"
#include "ui/NodeConnectionEditor.h"

NodeConnection::NodeConnection(NodeManager* nodeManager, Node* sourceNode, Node* destNode, ConnectionType ct) :
	BaseItem("Connection", false, false),
	nodeManager(nodeManager),
	connectionType(ct),
	sourceNode(nullptr),
	destNode(nullptr),
	activityLevel(0),
	connectionNotifier(5)
{
	setSourceNode(sourceNode);
	setDestNode(destNode);
}

NodeConnection::~NodeConnection()
{
	clearItem();
	setSourceNode(nullptr);
	setDestNode(nullptr);
}


void NodeConnection::setSourceNode(Node* node)
{
	if (node == sourceNode) return;

	if (sourceNode != nullptr)
	{
		sourceNode->removeNodeListener(this);
		sourceNode->removeInspectableListener(this);
		sourceNode->removeOutConnection(this);
	}

	sourceNode = node;

	if (sourceNode != nullptr)
	{
		sourceNode->addNodeListener(this);
		sourceNode->addInspectableListener(this);
		sourceNode->addOutConnection(this);

	}

	handleNodesUpdated();

	connectionNotifier.addMessage(new ConnectionEvent(ConnectionEvent::SOURCE_NODE_CHANGED, this));
}

void NodeConnection::setDestNode(Node* node)
{
	if (node == destNode) return;
	if (destNode != nullptr)
	{
		destNode->removeNodeListener(this);
		destNode->removeInspectableListener(this);
		destNode->removeInConnection(this);
	}

	destNode = node;

	if (destNode != nullptr)
	{
		destNode->addNodeListener(this);
		destNode->addInspectableListener(this);
		destNode->addInConnection(this);
	}

	handleNodesUpdated();

	connectionNotifier.addMessage(new ConnectionEvent(ConnectionEvent::DEST_NODE_CHANGED, this));
}

void NodeConnection::handleNodesUpdated()
{
	if (isCurrentlyLoadingData) return;

	if (sourceNode != nullptr && destNode != nullptr)
	{
		setNiceName(sourceNode->niceName + " > " + destNode->niceName);
	}
}


void NodeConnection::inspectableDestroyed(Inspectable* i)
{
	if (i == sourceNode || i == destNode)
	{
		if (i == sourceNode) setSourceNode(nullptr);
		else if (i == destNode) setSourceNode(nullptr);
	}
}

var NodeConnection::getJSONData()
{
	var data = BaseItem::getJSONData();
	if (sourceNode != nullptr) data.getDynamicObject()->setProperty("sourceNode", sourceNode->shortName);
	if (destNode != nullptr) data.getDynamicObject()->setProperty("destNode", destNode->shortName);
	data.getDynamicObject()->setProperty("connectionType", connectionType);
	return data;
}

void NodeConnection::loadJSONDataItemInternal(var data)
{
	if (data.hasProperty("sourceNode")) setSourceNode(nodeManager->getItemWithName(data.getProperty("sourceNode", "")));
	if (data.hasProperty("destNode")) setDestNode(nodeManager->getItemWithName(data.getProperty("destNode", "")));
	
	loadJSONDataConnectionInternal(data);
}

NodeAudioConnection::NodeAudioConnection(NodeManager* nodeManager, Node* sourceNode, Node* destNode) :
	NodeConnection(nodeManager, sourceNode, destNode, AUDIO)
{
	createDefaultConnections();
}

NodeAudioConnection::~NodeAudioConnection()
{
}

//AUDIO CONNECTION
void NodeAudioConnection::clearItem()
{
	clearConnections();
}

void NodeAudioConnection::handleNodesUpdated()
{
	NodeConnection::handleNodesUpdated();

	if (sourceNode != nullptr
		&& destNode != nullptr
		&& channelMap.size() == 0)
	{
		if (!isCurrentlyLoadingData) createDefaultConnections();
	}
	else clearConnections();
}

void NodeAudioConnection::connectChannels(int sourceChannel, int destChannel)
{
	if (sourceNode == nullptr || destNode == nullptr)
	{
		if (isCurrentlyLoadingData)
		{
			DBG("Bad connection, maybe node has changed typeString since last save. will be removed after loading");
			return;
		}
		else jassertfalse;
	}


	if (sourceChannel >= sourceNode->getNumAudioOutputs() || destChannel >= destNode->getNumAudioInputs())
	{
		if(isCurrentlyLoadingData)
		{
			ghostChannelMap.add({ sourceChannel, destChannel }); //one of the node may not be fully init, keep for later
		}
		else
		{
			LOGWARNING("Wrong connection, not connecting");
		}
		return;
	}

	channelMap.add({ sourceChannel, destChannel });
	ghostChannelMap.removeAllInstancesOf({ sourceChannel, destChannel });

	nodeManager->graph->addConnection({{sourceNode->nodeGraphID, sourceChannel}, { destNode->nodeGraphID, destChannel } });
	connectionNotifier.addMessage(new ConnectionEvent(ConnectionEvent::CHANNELS_CONNECTION_CHANGED, this));
	
}

void NodeAudioConnection::disconnectChannels(int sourceChannel, int destChannel, bool updateMap, bool notify)
{
	jassert(sourceNode != nullptr && destNode != nullptr);
	if (updateMap) channelMap.removeAllInstancesOf({ sourceChannel, destChannel });

	nodeManager->graph->removeConnection({ {sourceNode->nodeGraphID, sourceChannel}, { destNode->nodeGraphID, destChannel } });
	if (notify) connectionNotifier.addMessage(new ConnectionEvent(ConnectionEvent::CHANNELS_CONNECTION_CHANGED, this));
}

void NodeAudioConnection::createDefaultConnections()
{
	if (sourceNode == nullptr || destNode == nullptr) return;

	clearConnections();
	int defaultConnections = jmin(sourceNode->getNumAudioOutputs(), destNode->getNumAudioInputs());
	for (int i = 0; i < defaultConnections; i++) connectChannels(i, i);
}

void NodeAudioConnection::clearConnections()
{
	for (auto& c : channelMap) disconnectChannels(c.sourceChannel, c.destChannel, false, false);
	channelMap.clear();
	connectionNotifier.addMessage(new ConnectionEvent(ConnectionEvent::CHANNELS_CONNECTION_CHANGED, this));
}

void NodeAudioConnection::updateConnections()
{
	Array<ChannelMap> toRemove;

	//Remove phase
	for (auto& cm : channelMap) if (cm.destChannel > destNode->getNumAudioInputs() - 1 || cm.sourceChannel > sourceNode->getNumAudioOutputs() - 1) toRemove.add(cm);
	for (auto& rm : toRemove)
	{
		ghostChannelMap.addIfNotAlreadyThere({ rm.sourceChannel, rm.destChannel });
		disconnectChannels(rm.sourceChannel, rm.destChannel, true, false);
	}

	//Add ghost phase
	Array<ChannelMap> toAdd;
	for (auto& cm : ghostChannelMap)  if (cm.destChannel <= destNode->getNumAudioInputs() - 1 && cm.sourceChannel <= sourceNode->getNumAudioOutputs() - 1) toAdd.add(cm);
	for (auto& am : toAdd)
	{
		connectChannels(am.sourceChannel, am.destChannel);
	}

	if (!toRemove.isEmpty()) connectionNotifier.addMessage(new ConnectionEvent(ConnectionEvent::CHANNELS_CONNECTION_CHANGED, this));
}

void NodeAudioConnection::audioInputsChanged(Node* n)
{
	if (n == destNode) updateConnections();
}

void NodeAudioConnection::audioOutputsChanged(Node* n)
{
	if (n == sourceNode) updateConnections();
}

var NodeAudioConnection::getChannelMapData()
{
	var chData;
	for (auto& c : channelMap)
	{
		var ch;
		ch.append(c.sourceChannel);
		ch.append(c.destChannel);
		chData.append(ch);
	}

	return chData;
}

void NodeAudioConnection::loadChannelMapData(var data)
{
	if (!data.isVoid()) clearConnections();
	for (int i = 0; i < data.size(); i++)
	{
		connectChannels(data[i][0], data[i][1]);
	}
}

InspectableEditor* NodeAudioConnection::getEditor(bool isRoot)
{
	return new NodeAudioConnectionEditor(this, isRoot);
}


var NodeAudioConnection::getJSONData()
{
	var data = NodeConnection::getJSONData();

	data.getDynamicObject()->setProperty("channels", getChannelMapData());

	if (ghostChannelMap.size() > 0)
	{
		var ghostData;
		for (auto& c : ghostChannelMap)
		{
			var ch;
			ch.append(c.sourceChannel);
			ch.append(c.destChannel);
			ghostData.append(ch);
		}

		data.getDynamicObject()->setProperty("ghostChannels", ghostData);
	}

	return data;
}

void NodeAudioConnection::loadJSONDataConnectionInternal(var data)
{
	clearConnections();

	var ghostData = data.getProperty("ghostChannels", var());
	for (int i = 0; i < ghostData.size(); i++)
	{
		//DBG("Ghost channel set > " << (int)ghostData[i][0] << " > " <<(int) ghostData[i][1]);
		ghostChannelMap.add({ ghostData[i][0], ghostData[i][1] });
	}

	loadChannelMapData(data.getProperty("channels", var()));
}

NodeMIDIConnection::NodeMIDIConnection(NodeManager* nodeManager, Node* sourceNode, Node* destNode) :
	NodeConnection(nodeManager, sourceNode, destNode, MIDI)
{
}

NodeMIDIConnection::~NodeMIDIConnection()
{
}

void NodeMIDIConnection::clearItem()
{
	NodeConnection::clearItem();

	if (destNode != nullptr)
	{
		MidiBuffer clearBuffer;
		for (int i = 1; i <= 16; i++) clearBuffer.addEvent(MidiMessage::allNotesOff(i), 1);
		destNode->receiveMIDIFromInput(sourceNode, clearBuffer);
	}
}
