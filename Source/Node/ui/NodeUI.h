/*
  ==============================================================================

    NodeUI.h
    Created: 15 Nov 2020 9:26:53am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

class NodeUI :
    public ItemUI<Node>
{
public:
    NodeUI(Node * node);
    virtual ~NodeUI();
};