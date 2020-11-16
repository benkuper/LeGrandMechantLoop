/*
  ==============================================================================

    NodeConnector.h
    Created: 16 Nov 2020 3:03:26pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "../NodeConnection.h"

class NodeConnector :
    public Component,
    public SettableTooltipClient
{
public:
    NodeConnector(Node * n, bool isInput);
    ~NodeConnector();

    bool isInput;
    Node* node;

    void paint(Graphics& g) override;
    void updateTooltip();
};