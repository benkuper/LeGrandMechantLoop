/*
  ==============================================================================

    MixerNode.h
    Created: 15 Nov 2020 8:42:42am
    Author:  bkupe

  ==============================================================================
*/

#pragma once
#include "../../Node.h"

class MixerNode :
    public Node
{
public:
    MixerNode(var params = var());
    ~MixerNode();

    String getTypeString() const override { return getTypeStringStatic(); }
    static const String getTypeStringStatic() { return "Mixer"; }

    static MixerNode* create(var params) { return new MixerNode(params); }
};