/*
  ==============================================================================

    NodeConnectionManager.h
    Created: 16 Nov 2020 10:00:12am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "NodeConnection.h"

class NodeConnectionManager :
    public BaseManager<NodeConnection>
{
public:
    NodeConnectionManager();
    ~NodeConnectionManager();

    NodeConnection* createConnectionForType(NodeConnection::ConnectionType t);

    void addConnection(Node * sourceNode, Node * destNode, NodeConnection::ConnectionType connectionType, var channelMapData = var());
    Array<UndoableAction*> getAddConnectionUndoableAction(Node* sourceNode, Node* destNode, NodeConnection::ConnectionType connectionType, var channelMapData = var());
    virtual NodeConnection* getConnectionForSourceAndDest(Node* sourceNode, Node* destNode, NodeConnection::ConnectionType connectionType);
    Array<UndoableAction*> getRemoveAllLinkedConnectionsActions(Array<Node*> itemsToRemove);

    NodeConnection* addItemFromData(var data, bool addToUndo = true) override;
    Array<NodeConnection *> addItemsFromData(var data, bool addToUndo = true) override;

    void afterLoadJSONDataInternal() override;
};