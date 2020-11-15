/*
  ==============================================================================

    NodeManagerViewUI.h
    Created: 15 Nov 2020 8:40:22am
    Author:  bkupe

  ==============================================================================
*/

#pragma once
#include "../NodeManager.h"
#include "NodeViewUI.h"

class NodeManagerViewUI :
    public BaseManagerShapeShifterViewUI<NodeManager, Node, NodeViewUI>
{
public:
    NodeManagerViewUI(StringRef name);
    ~NodeManagerViewUI();


    NodeViewUI* createUIForItem(Node * n) override;

    static NodeManagerViewUI* create(const String& name) { return new NodeManagerViewUI(name); }
};
