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
        sourceNode->removeInspectableListener(this);
    }

    sourceNode = node;

    if (sourceNode != nullptr)
    {
        sourceNode->addInspectableListener(this);
        if(!isCurrentlyLoadingData && destNode != nullptr) createDefaultConnections();
    }

    //changed
}

void NodeConnection::setDestNode(Node* node)
{
    if (node == destNode) return;
    if (destNode != nullptr)
    {
        destNode->removeInspectableListener(this);
    }

    destNode = node;

    if (destNode != nullptr)
    {
        destNode->addInspectableListener(this);
        if(!isCurrentlyLoadingData && sourceNode != nullptr) createDefaultConnections();
    }
}

void NodeConnection::connectChannels(int sourceChannel, int destChannel)
{
    jassert(sourceNode != nullptr && destNode != nullptr);
    channelMap.add({ sourceChannel, destChannel });
    AudioManager::getInstance()->graph.addConnection({ { sourceNode->nodeGraphID, sourceChannel }, { destNode->nodeGraphID,  destChannel } });
}

void NodeConnection::disconnectChannels(int sourceChannel, int destChannel, bool updateMap)
{
    jassert(sourceNode != nullptr && destNode != nullptr);
    if (updateMap) channelMap.removeAllInstancesOf({ sourceChannel, destChannel });
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
    for(auto & c : channelMap) disconnectChannels(c.sourceChannel, c.destChannel, false);
    channelMap.clear();
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
    var chData;
    for(auto & c : channelMap)
    {
        var ch;
        ch.append(c.sourceChannel);
        ch.append(c.destChannel);
        chData.append(ch);
    }

    data.getDynamicObject()->setProperty("channels", chData);
    return data;
}

void NodeConnection::loadJSONDataItemInternal(var data)
{
    if (data.hasProperty("sourceNode")) setSourceNode((Node*)RootNodeManager::getInstance()->getControllableContainerForAddress(data.getProperty("sourceNode", "")));
    if (data.hasProperty("destNode")) setDestNode((Node*)RootNodeManager::getInstance()->getControllableContainerForAddress(data.getProperty("destNode", "")));
    clearConnections();
    var chData = data.getProperty("channels", var());
    for (int i = 0; i < chData.size(); i++)
    {
        connectChannels(chData[i][0], chData[i][1]);
    }
}

InspectableEditor* NodeConnection::getEditor(bool isRoot)
{
    return new NodeConnectionEditor(this, isRoot);
}
