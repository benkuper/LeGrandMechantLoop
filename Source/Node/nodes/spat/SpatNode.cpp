/*
  ==============================================================================

    SpatNode.cpp
    Created: 15 Nov 2020 8:42:35am
    Author:  bkupe

  ==============================================================================
*/

#include "SpatNode.h"

SpatNode::SpatNode(var params) :
    Node(getTypeString(), SPAT, new NodeAudioProcessor(this), params)

{
}

SpatNode::~SpatNode()
{
}
