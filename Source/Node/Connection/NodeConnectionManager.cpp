/*
  ==============================================================================

    NodeConnectionManager.cpp
    Created: 16 Nov 2020 10:00:12am
    Author:  bkupe

  ==============================================================================
*/

#include "NodeConnectionManager.h"

NodeConnectionManager::NodeConnectionManager(AudioProcessorGraph* graph) :
    BaseManager("Connections"),
    graph(graph)
{
}

NodeConnectionManager::~NodeConnectionManager()
{
}

NodeConnection* NodeConnectionManager::createConnectionForType(NodeConnection::ConnectionType t)
{
    if (t == NodeConnection::AUDIO) return new NodeAudioConnection();
    else if (t == NodeConnection::MIDI) return new NodeMIDIConnection();
    return nullptr;
}

void NodeConnectionManager::addConnection(Node* sourceNode, Node* destNode, NodeConnection::ConnectionType connectionType, var channelMapData)
{
    UndoMaster::getInstance()->performActions("Add Connection", getAddConnectionUndoableAction(sourceNode, destNode, connectionType, channelMapData));
}

Array<UndoableAction*> NodeConnectionManager::getAddConnectionUndoableAction(Node* sourceNode, Node* destNode, NodeConnection::ConnectionType connectionType, var channelMapData)
{
    if (getConnectionForSourceAndDest(sourceNode, destNode, connectionType))
    {
        LOGWARNING("Connection already exists !");
        return Array<UndoableAction*>();
    }

    NodeConnection* connection = nullptr;
    if (connectionType == NodeConnection::AUDIO)
    {
        NodeAudioConnection* nc = new NodeAudioConnection(sourceNode, destNode);
        if (channelMapData.isObject()) nc->loadChannelMapData(channelMapData);
        connection = nc;
    }
    else if (connectionType == NodeConnection::MIDI)
    {
        connection = new NodeMIDIConnection(sourceNode, destNode);
    }

    if (connection == nullptr) return Array<UndoableAction*>();

    return getAddItemUndoableAction(connection);
}

NodeConnection* NodeConnectionManager::getConnectionForSourceAndDest(Node* sourceNode, Node* destNode, NodeConnection::ConnectionType connectionType)
{
    for (auto& c : items) if (c->sourceNode == sourceNode && c->destNode == destNode && c->connectionType == connectionType) return c;
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

NodeConnection* NodeConnectionManager::addItemFromData(var data, bool addToUndo)
{
    NodeConnection::ConnectionType t = (NodeConnection::ConnectionType)(int)data.getProperty("connectionType", NodeConnection::AUDIO);
    if (NodeConnection * nc = createConnectionForType(t)) return addItem(nc, data, addToUndo);;
    return nullptr;
}

Array<NodeConnection*> NodeConnectionManager::addItemsFromData(var data, bool addToUndo)
{
    Array<NodeConnection*> itemsToAdd;

    for (int i = 0; i < data.size(); i++)
    {
        NodeConnection::ConnectionType t = (NodeConnection::ConnectionType)(int)data[i].getProperty("connectionType", NodeConnection::AUDIO);
        if (NodeConnection* nc = createConnectionForType(t)) itemsToAdd.add(nc);
    }
    return addItems(itemsToAdd, data, addToUndo);
}


void NodeConnectionManager::addItemInternal(NodeConnection* nc, var data)
{
    if (NodeAudioConnection* nac = dynamic_cast<NodeAudioConnection*>(nc))
    {
        nac->addAudioConnectionListener(this);
        for (auto& cm : nac->channelMap) //add existing channel mappings
        {
            graph->addConnection({ {nac->sourceNode->nodeGraphID, cm.sourceChannel}, { nac->destNode->nodeGraphID, cm.destChannel } });
        }
    }
}

void NodeConnectionManager::removeItemInternal(NodeConnection* nc)
{
    if (NodeAudioConnection* nac = dynamic_cast<NodeAudioConnection*>(nc))
    {
        nac->removeAudioConnectionListener(this);
    }
}

void NodeConnectionManager::askForConnectChannels(NodeConnection* nc, int sourceChannel, int destChannel)
{
    graph->addConnection({ {nc->sourceNode->nodeGraphID, sourceChannel},{nc->destNode->nodeGraphID, destChannel} });
}

void NodeConnectionManager::askForDisconnectChannels(NodeConnection* nc, int sourceChannel, int destChannel)
{
    graph->removeConnection({ {nc->sourceNode->nodeGraphID, sourceChannel},{nc->destNode->nodeGraphID, destChannel} });
}

void NodeConnectionManager::afterLoadJSONDataInternal()
{
    BaseManager::afterLoadJSONDataInternal();

    Array<NodeConnection*> toRemove;
    for (auto& i : items) if (i->sourceNode == nullptr || i->destNode == nullptr) toRemove.add(i);
    for (auto& i : toRemove) removeItem(i);
}
