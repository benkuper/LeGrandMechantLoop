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
#include "nodes/looper/MIDILooperNode.h"
#include "nodes/mixer/MixerNode.h"
#include "nodes/spat/SpatNode.h"
#include "nodes/vst/VSTNode.h"
#include "nodes/container/ContainerNode.h"

juce_ImplementSingleton(NodeFactory)

NodeFactory::NodeFactory()
{
    defs.add(Definition::createDef("", ContainerProcessor::getTypeStringStatic(), &GenericNode<ContainerProcessor>::create));
    defs.add(Definition::createDef("Audio", AudioInputProcessor::getTypeStringStatic(), &GenericNode<AudioInputProcessor>::create));
    defs.add(Definition::createDef("Audio", AudioOutputProcessor::getTypeStringStatic(), &GenericNode<AudioOutputProcessor>::create));
    defs.add(Definition::createDef("Audio", AudioLooperProcessor::getTypeStringStatic(), &GenericNode<AudioLooperProcessor>::create));
    defs.add(Definition::createDef("Audio", MixerProcessor::getTypeStringStatic(), &GenericNode<MixerProcessor>::create));
    defs.add(Definition::createDef("Audio", SpatProcessor::getTypeStringStatic(), &GenericNode<SpatProcessor>::create));
    defs.add(Definition::createDef("Audio", VSTProcessor::getTypeStringStatic(), &GenericNode<VSTProcessor>::create));
    defs.add(Definition::createDef("MIDI", MIDILooperProcessor::getTypeStringStatic(), &GenericNode<MIDILooperProcessor>::create));
}
