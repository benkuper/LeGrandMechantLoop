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

NodeConnection::NodeConnection(Node* sourceNode, Node* destNode) :
	BaseItem("Connection", true, false),
	sourceNode(nullptr),
	destNode(nullptr),
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

void NodeConnection::clearItem()
{
	clearConnections();
}

void NodeConnection::setSourceNode(Node* node)
{
	if (node == sourceNode) return;

	if (sourceNode != nullptr)
	{
		sourceNode->removeNodeListener(this);
		sourceNode->removeInspectableListener(this);
	}

	sourceNode = node;

	if (sourceNode != nullptr)
	{
		sourceNode->addNodeListener(this);
		sourceNode->addInspectableListener(this);
		if (!isCurrentlyLoadingData && destNode != nullptr) createDefaultConnections();
	}

	connectionNotifier.addMessage(new ConnectionEvent(ConnectionEvent::SOURCE_NODE_CHANGED, this));
}

void NodeConnection::setDestNode(Node* node)
{
	if (node == destNode) return;
	if (destNode != nullptr)
	{
		destNode->removeNodeListener(this);
		destNode->removeInspectableListener(this);
	}

	destNode = node;

	if (destNode != nullptr)
	{
		destNode->addNodeListener(this);
		destNode->addInspectableListener(this);
		if (!isCurrentlyLoadingData && sourceNode != nullptr) createDefaultConnections();
	}

	connectionNotifier.addMessage(new ConnectionEvent(ConnectionEvent::DEST_NODE_CHANGED, this));
}

void NodeConnection::connectChannels(int sourceChannel, int destChannel)
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
	

	if (sourceChannel >= sourceNode->numOutputs || destChannel >= destNode->numInputs)
	{
		LOGWARNING("Wrong connection, not connecting");
		return;
	}

	channelMap.add({ sourceChannel, destChannel });
	ghostChannelMap.removeAllInstancesOf({ sourceChannel, destChannel });

	connectionNotifier.addMessage(new ConnectionEvent(ConnectionEvent::CHANNELS_CONNECTION_CHANGED, this));
	AudioManager::getInstance()->graph.addConnection({ { sourceNode->nodeGraphID, sourceChannel }, { destNode->nodeGraphID,  destChannel } });
}

void NodeConnection::disconnectChannels(int sourceChannel, int destChannel, bool updateMap, bool notify)
{
	jassert(sourceNode != nullptr && destNode != nullptr);
	if (updateMap) channelMap.removeAllInstancesOf({ sourceChannel, destChannel });
	if (notify) connectionNotifier.addMessage(new ConnectionEvent(ConnectionEvent::CHANNELS_CONNECTION_CHANGED, this));
	AudioManager::getInstance()->graph.removeConnection({ { sourceNode->nodeGraphID, sourceChannel }, { destNode->nodeGraphID,  destChannel } });
}

void NodeConnection::createDefaultConnections()
{
	if (sourceNode == nullptr || destNode == nullptr) return;

	clearConnections();
	int defaultConnections = jmin(sourceNode->numOutputs, destNode->numInputs);
	for (int i = 0; i < defaultConnections; i++) connectChannels(i, i);
}

void NodeConnection::clearConnections()
{
	for (auto& c : channelMap) disconnectChannels(c.sourceChannel, c.destChannel, false, false);
	channelMap.clear();
	connectionNotifier.addMessage(new ConnectionEvent(ConnectionEvent::CHANNELS_CONNECTION_CHANGED, this));
}

void NodeConnection::updateConnections()
{
	Array<ChannelMap> toRemove;

	//Remove phase
	for (auto& cm : channelMap) if (cm.destChannel > destNode->numInputs - 1 || cm.sourceChannel > sourceNode->numOutputs - 1) toRemove.add(cm);
	for (auto& rm : toRemove)
	{
		ghostChannelMap.addIfNotAlreadyThere({ rm.sourceChannel, rm.destChannel });
		disconnectChannels(rm.sourceChannel, rm.destChannel, true, false);
	}

	//Add ghost phase
	Array<ChannelMap> toAdd;
	for (auto& cm : ghostChannelMap)  if (cm.destChannel <= destNode->numInputs - 1 && cm.sourceChannel <= sourceNode->numOutputs - 1) toAdd.add(cm);
	for (auto& am : toAdd)
	{
		connectChannels(am.sourceChannel, am.destChannel);
	}

	if (!toRemove.isEmpty()) connectionNotifier.addMessage(new ConnectionEvent(ConnectionEvent::CHANNELS_CONNECTION_CHANGED, this));
}

void NodeConnection::audioInputsChanged(Node* n)
{
	if (n == destNode) updateConnections();
}

void NodeConnection::audioOutputsChanged(Node* n)
{
	if (n == sourceNode) updateConnections();
}

void NodeConnection::inspectableDestroyed(Inspectable* i)
{
	if (i == sourceNode || i == destNode)
	{
		clearConnections();
		if (i == sourceNode) setSourceNode(nullptr);
		else if (i == destNode) setSourceNode(nullptr);
	}
}

var NodeConnection::getJSONData()
{
	var data = BaseItem::getJSONData();
	if (sourceNode != nullptr) data.getDynamicObject()->setProperty("sourceNode", sourceNode->getControlAddress(RootNodeManager::getInstance()));
	if (destNode != nullptr) data.getDynamicObject()->setProperty("destNode", destNode->getControlAddress(RootNodeManager::getInstance()));

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

void NodeConnection::loadJSONDataItemInternal(var data)
{
	if (data.hasProperty("sourceNode")) setSourceNode((Node*)RootNodeManager::getInstance()->getControllableContainerForAddress(data.getProperty("sourceNode", "")));
	if (data.hasProperty("destNode")) setDestNode((Node*)RootNodeManager::getInstance()->getControllableContainerForAddress(data.getProperty("destNode", "")));
	clearConnections();

	var ghostData = data.getProperty("ghostChannels", var());
	for (int i = 0; i < ghostData.size(); i++)
	{
		ghostChannelMap.add({ ghostData[i][0], ghostData[i][1] });
	}

	loadChannelMapData(data.getProperty("channels", var()));
}

var NodeConnection::getChannelMapData()
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

void NodeConnection::loadChannelMapData(var data)
{
	if (!data.isVoid()) clearConnections();
	for (int i = 0; i < data.size(); i++)
	{
		connectChannels(data[i][0], data[i][1]);
	}
}

InspectableEditor* NodeConnection::getEditor(bool isRoot)
{
	return new NodeConnectionEditor(this, isRoot);
}
