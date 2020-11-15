/*
  ==============================================================================

    VSTNode.cpp
    Created: 15 Nov 2020 8:42:29am
    Author:  bkupe

  ==============================================================================
*/

#include "VSTNode.h"

VSTNode::VSTNode(var params) :
    Node(getTypeString(), VST, new NodeAudioProcessor(this), params)
{
}

VSTNode::~VSTNode()
{
}
