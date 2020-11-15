/*
  ==============================================================================

    LooperNode.h
    Created: 15 Nov 2020 8:43:03am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "../../Node.h"

class LooperNode :
    public Node
{
public:
    LooperNode(var params = var());
    ~LooperNode();

    String getTypeString() const override { return getTypeStringStatic(); }
    static const String getTypeStringStatic() { return "Looper"; }

    static LooperNode* create(var params) { return new LooperNode(params); }
};