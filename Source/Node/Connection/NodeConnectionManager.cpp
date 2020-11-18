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

void NodeConnectionManager::removeItemInternal(NodeConnection* c)
{
    c->clearConnections();
}

void NodeConnectionManager::addConnection(Node* sourceNode, Node* destNode)
{
    NodeConnection* connection = new NodeConnection(sourceNode, destNode);
    addItem(connection);
}
