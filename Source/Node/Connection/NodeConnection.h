/*
  ==============================================================================

    NodeConnection.h
    Created: 16 Nov 2020 10:00:05am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"
class Node;

class NodeConnection :
    public BaseItem
{
public:
    NodeConnection(Node * sourceNode = nullptr, Node * destNode = nullptr);
    ~NodeConnection();

    Node * sourceNode;
    Node * destNode;


    void setSourceNode(Node* node);
    void setDestNode(Node* node);
   
    DECLARE_ASYNC_EVENT(NodeConnection, Connection, connection, ENUM_LIST(SOURCE_NODE_CHANGED, DEST_NODE_CHANGED))
};