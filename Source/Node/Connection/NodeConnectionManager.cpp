/*
  ==============================================================================

    NodeConnectionManager.cpp
    Created: 16 Nov 2020 10:00:12am
    Author:  bkupe

  ==============================================================================
*/

#include "NodeConnectionManager.h"

NodeConnectionManager::NodeConnectionManager() :
    BaseManager("Connections")
{
}

NodeConnectionManager::~NodeConnectionManager()
{
}

void NodeConnectionManager::addConnection(Node* sourceNode, Node* destNode, var channelMapData)
{
    if (getConnectionForSourceAndDest(sourceNode, destNode))
    {
        LOGWARNING("Connection already exists !");
        return;
    }

    NodeConnection* connection = new NodeConnection(sourceNode, destNode);
    if (channelMapData.isObject()) connection->loadChannelMapData(channelMapData);
    addItem(connection);
}

Array<UndoableAction*> NodeConnectionManager::getAddConnectionUndoableAction(Node* sourceNode, Node* destNode, var channelMapData)
{
    if (getConnectionForSourceAndDest(sourceNode, destNode))
    {
        LOGWARNING("Connection already exists !");
        return Array<UndoableAction*>();
    }

    NodeConnection* connection = new NodeConnection(sourceNode, destNode);
    if (channelMapData.isObject()) connection->loadChannelMapData(channelMapData);
    return getAddItemUndoableAction(connection);
}

NodeConnection* NodeConnectionManager::getConnectionForSourceAndDest(Node* sourceNode, Node* destNode)
{
    for (auto& c : items) if (c->sourceNode == sourceNode && c->destNode == destNode) return c;
    return nullptr;
}

Array<UndoableAction*> NodeConnectionManager::getRemoveAllLinkedConnectionsActions(Array<Node*> itemsToRemove)
{
    Array<NodeConnection*> connectionsToRemove;

    for (auto& c : items)
    {
        if (itemsToRemove.contains(c->sourceNode) || itemsToRemove.contains(c->destNode)) connectionsToRemove.addIfNotAlreadyThere(c);
    }
    
    Array<UndoableAction*> actions;
    actions.addArray(getRemoveItemsUndoableAction(connectionsToRemove));
    return actions;
}

void NodeConnectionManager::afterLoadJSONDataInternal()
{
    BaseManager::afterLoadJSONDataInternal();

    Array<NodeConnection*> toRemove;
    for (auto& i : items) if (i->sourceNode == nullptr || i->destNode == nullptr) toRemove.add(i);
    for (auto& i : toRemove) removeItem(i);
}
