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
    public BaseItem,
    public Inspectable::InspectableListener
{
public:
    NodeConnection(Node * sourceNode = nullptr, Node * destNode = nullptr);
    ~NodeConnection();

    Node * sourceNode;
    Node * destNode;

    struct ChannelMap {
        int sourceChannel;
        int destChannel;

        inline bool operator == (const ChannelMap& c2) const { return sourceChannel == c2.sourceChannel && destChannel == c2.destChannel; }
    };
    Array<ChannelMap> channelMap;

    void clearItem() override;

    void setSourceNode(Node* node);
    void setDestNode(Node* node);

    void connectChannels(int sourceChannel, int destChannel);
    void disconnectChannels(int sourceChannel, int destChannel, bool updateMap = true);
    void createDefaultConnections();
    void clearConnections();


    void inspectableDestroyed(Inspectable *) override;

    var getJSONData() override;
    void loadJSONDataItemInternal(var data) override;


    InspectableEditor* getEditor(bool isRoot) override;
    
    DECLARE_ASYNC_EVENT(NodeConnection, Connection, connection, ENUM_LIST(SOURCE_NODE_CHANGED, DEST_NODE_CHANGED, CHANNELS_CONNECTION_CHANGED))
};