/*
  ==============================================================================

    NodeConnector.h
    Created: 16 Nov 2020 3:03:26pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "../NodeConnection.h"

class NodeViewUI;

class NodeConnector :
    public Component,
    public SettableTooltipClient
{
public:
    NodeConnector(NodeViewUI * n, bool isInput, NodeConnection::ConnectionType connectionType);
    ~NodeConnector();

    NodeConnection::ConnectionType connectionType;
    bool isInput;
    NodeViewUI * nodeViewUI;

    void paint(Graphics& g) override;
    void update();
};