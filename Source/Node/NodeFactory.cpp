/*
  ==============================================================================

    NodeFactory.cpp
    Created: 15 Nov 2020 8:56:42am
    Author:  bkupe

  ==============================================================================
*/

#include "NodeFactory.h"
#include "nodes/io/IONode.h"
#include "nodes/looper/LooperNode.h"
#include "nodes/mixer/MixerNode.h"
#include "nodes/spat/SpatNode.h"
#include "nodes/vst/VSTNode.h"

juce_ImplementSingleton(NodeFactory)

NodeFactory::NodeFactory()
{
    defs.add(Definition::createDef("", AudioInputNode::getTypeStringStatic(), &AudioInputNode::create));
    defs.add(Definition::createDef("", AudioOutputNode::getTypeStringStatic(), &AudioOutputNode::create));
    defs.add(Definition::createDef("", LooperNode::getTypeStringStatic(), &LooperNode::create));
    defs.add(Definition::createDef("", MixerNode::getTypeStringStatic(), &MixerNode::create));
    defs.add(Definition::createDef("", SpatNode::getTypeStringStatic(), &SpatNode::create));
    defs.add(Definition::createDef("", VSTNode::getTypeStringStatic(), &VSTNode::create));
}
