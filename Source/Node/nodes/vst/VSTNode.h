/*
  ==============================================================================

    VSTNode.h
    Created: 15 Nov 2020 8:42:29am
    Author:  bkupe

  ==============================================================================
*/

#pragma once
#include "../../Node.h"

class VSTNode :
    public Node
{
public:
    VSTNode(var params = var());
    ~VSTNode();

    String getTypeString() const override { return getTypeStringStatic(); }
    static const String getTypeStringStatic() { return "VST"; }

    static VSTNode* create(var params) { return new VSTNode(params); }
};
