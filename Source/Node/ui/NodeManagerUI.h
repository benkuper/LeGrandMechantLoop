/*
  ==============================================================================

    NodeManagerUI.h
    Created: 15 Nov 2020 8:40:16am
    Author:  bkupe

  ==============================================================================
*/

#pragma once
#include "../NodeManager.h"
#include "NodeUI.h"

class NodeManagerUI :
    public BaseManagerShapeShifterUI<NodeManager, Node, NodeUI>
{
public:
    NodeManagerUI(StringRef name);
    ~NodeManagerUI();

    static NodeManagerUI* create(const String& name) { return new NodeManagerUI(name); }
};