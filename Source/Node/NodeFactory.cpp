/*
  ==============================================================================

    NodeFactory.cpp
    Created: 15 Nov 2020 8:56:42am
    Author:  bkupe

  ==============================================================================
*/

#include "NodeFactory.h"
#include "nodes/io/IONode.h"
#include "nodes/looper/AudioLooperNode.h"
#include "nodes/mixer/MixerNode.h"
#include "nodes/spat/SpatNode.h"
#include "nodes/vst/VSTNode.h"

juce_ImplementSingleton(NodeFactory)

NodeFactory::NodeFactory()
{
    defs.add(Definition::createDef("", AudioInputProcessor::getTypeStringStatic(), &GenericNode<AudioInputProcessor>::create));
    defs.add(Definition::createDef("", AudioOutputProcessor::getTypeStringStatic(), &GenericNode<AudioOutputProcessor>::create));
    defs.add(Definition::createDef("", AudioLooperProcessor::getTypeStringStatic(), &GenericNode<AudioLooperProcessor>::create));
    defs.add(Definition::createDef("", MixerProcessor::getTypeStringStatic(), &GenericNode<MixerProcessor>::create));
    defs.add(Definition::createDef("", SpatProcessor::getTypeStringStatic(), &GenericNode<SpatProcessor>::create));
    defs.add(Definition::createDef("", VSTProcessor::getTypeStringStatic(), &GenericNode<VSTProcessor>::create));
}
