/*
  ==============================================================================

    NodeConnection.cpp
    Created: 16 Nov 2020 10:00:05am
    Author:  bkupe

  ==============================================================================
*/

#include "NodeConnection.h"

NodeConnection::NodeConnection(Node* sourceNode, Node* destNode) :
    BaseItem("Connection", true, false),
    sourceNode(sourceNode),
    destNode(destNode),
    connectionNotifier(5)
{

}

NodeConnection::~NodeConnection()
{
}

void NodeConnection::setSourceNode(Node* node)
{
    if (node == sourceNode) return;
    if (sourceNode != nullptr)
    {

    }

    sourceNode = node;

    if (sourceNode != nullptr)
    {

    }

    //changed
}

void NodeConnection::setDestNode(Node* node)
{
    if (node == destNode) return;
    if (destNode != nullptr)
    {

    }

    destNode = node;

    if (destNode != nullptr)
    {

    }
}
