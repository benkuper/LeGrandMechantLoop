/*
  ==============================================================================

    SpatNode.h
    Created: 15 Nov 2020 8:42:35am
    Author:  bkupe

  ==============================================================================
*/

#pragma once
#include "../../Node.h"

class SpatNode :
    public Node
{
public:
    SpatNode(var params = var());
    ~SpatNode();

    String getTypeString() const override { return getTypeStringStatic(); }
    static const String getTypeStringStatic() { return "Spat"; }

    static SpatNode* create(var params) { return new SpatNode(params); }
};
