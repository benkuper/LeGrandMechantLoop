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
    defs.add(Definition::createDef("", AudioInputProcessor::getTypeStringStatic(), &GenericAudioNode<AudioInputProcessor>::create));
    defs.add(Definition::createDef("", AudioOutputProcessor::getTypeStringStatic(), &GenericAudioNode<AudioOutputProcessor>::create));
    defs.add(Definition::createDef("", LooperProcessor::getTypeStringStatic(), &GenericAudioNode<LooperProcessor>::create));
    defs.add(Definition::createDef("", MixerProcessor::getTypeStringStatic(), &GenericAudioNode<MixerProcessor>::create));
    defs.add(Definition::createDef("", SpatProcessor::getTypeStringStatic(), &GenericAudioNode<SpatProcessor>::create));
    defs.add(Definition::createDef("", VSTProcessor::getTypeStringStatic(), &GenericAudioNode<VSTProcessor>::create));
}
