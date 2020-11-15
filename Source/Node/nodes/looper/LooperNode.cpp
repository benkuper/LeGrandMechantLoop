/*
  ==============================================================================

    LooperNode.cpp
    Created: 15 Nov 2020 8:43:03am
    Author:  bkupe

  ==============================================================================
*/

#include "LooperNode.h"

LooperNode::LooperNode(var params) :
    Node(getTypeString(), LOOPER, new NodeAudioProcessor(this), params)
{
}

LooperNode::~LooperNode()
{
}
