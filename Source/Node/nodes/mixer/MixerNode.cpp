/*
  ==============================================================================

    MixerNode.cpp
    Created: 15 Nov 2020 8:42:42am
    Author:  bkupe

  ==============================================================================
*/

#include "MixerNode.h"

MixerNode::MixerNode(var params) :
    Node(getTypeString(), MIXER, new NodeAudioProcessor(this), params)
{
}

MixerNode::~MixerNode()
{
}
