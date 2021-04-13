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
#include "nodes/io/MIDIIONode.h"
#include "nodes/mixer/MixerNode.h"
#include "nodes/spat/SpatNode.h"
#include "nodes/vst/VSTNode.h"
#include "nodes/container/ContainerNode.h"
#include "nodes/router/AudioRouterNode.h"

juce_ImplementSingleton(NodeFactory)

NodeFactory::NodeFactory()
{
    defs.add(Definition::createDef<ContainerNode>("", ContainerNode::getTypeStringStatic()));
    defs.add(Definition::createDef<AudioInputNode>("Audio", AudioInputNode::getTypeStringStatic()));
    defs.add(Definition::createDef<AudioOutputNode>("Audio", AudioOutputNode::getTypeStringStatic()));
    defs.add(Definition::createDef<AudioLooperNode>("Audio", AudioLooperNode::getTypeStringStatic()));
    defs.add(Definition::createDef<VSTNode>("Audio", VSTNode::getTypeStringStatic()));
    defs.add(Definition::createDef<MixerNode>("Audio", MixerNode::getTypeStringStatic()));
    defs.add(Definition::createDef<SpatNode>("Audio", SpatNode::getTypeStringStatic()));
    defs.add(Definition::createDef<AudioRouterNode>("Audio", AudioRouterNode::getTypeStringStatic()));

    defs.add(Definition::createDef<MIDILooperNode>("MIDI", MIDILooperNode::getTypeStringStatic()));
    defs.add(Definition::createDef<MIDIIONode>("MIDI", MIDIIONode::getTypeStringStatic()));
}
