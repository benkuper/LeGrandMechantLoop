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

    void addConnection(Node * sourceNode, Node * destNode, var channelMapData = var());
    Array<UndoableAction*> getAddConnectionUndoableAction(Node* sourceNode, Node* destNode, var channelMapData = var());

    virtual NodeConnection* getConnectionForSourceAndDest(Node* sourceNode, Node* destNode);

    Array<UndoableAction*> getRemoveAllLinkedConnectionsActions(Array<Node*> itemsToRemove);
};