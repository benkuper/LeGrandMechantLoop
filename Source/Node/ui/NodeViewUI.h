/*
  ==============================================================================

    NodeViewUI.h
    Created: 15 Nov 2020 9:26:57am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "../Node.h"

class NodeViewUI :
    public BaseItemUI<Node>
{
public:
    NodeViewUI(Node* node);
    virtual ~NodeViewUI();
};