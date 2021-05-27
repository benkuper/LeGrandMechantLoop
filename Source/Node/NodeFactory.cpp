/*
  ==============================================================================

    NodeFactory.cpp
    Created: 15 Nov 2020 8:56:42am
    Author:  bkupe

  ==============================================================================
*/

juce_ImplementSingleton(NodeFactory)

NodeFactory::NodeFactory()
{
    defs.add(Definition::createDef<ContainerNode>("", ContainerNode::getTypeStringStatic()));
    defs.add(Definition::createDef<AudioInputNode>("Audio", AudioInputNode::getTypeStringStatic()));
    defs.add(Definition::createDef<AudioOutputNode>("Audio", AudioOutputNode::getTypeStringStatic()));
    defs.add(Definition::createDef<AudioLooperNode>("Audio", AudioLooperNode::getTypeStringStatic()));
    defs.add(Definition::createDef<SamplerNode>("Audio", SamplerNode::getTypeStringStatic()));
    defs.add(Definition::createDef<VSTNode>("Audio", VSTNode::getTypeStringStatic()));
    defs.add(Definition::createDef<MixerNode>("Audio", MixerNode::getTypeStringStatic()));
    defs.add(Definition::createDef<SpatNode>("Audio", SpatNode::getTypeStringStatic()));
    defs.add(Definition::createDef<AudioRouterNode>("Audio", AudioRouterNode::getTypeStringStatic()));
    defs.add(Definition::createDef<RecorderNode>("Audio", RecorderNode::getTypeStringStatic()));

    defs.add(Definition::createDef<MIDILooperNode>("MIDI", MIDILooperNode::getTypeStringStatic()));
    defs.add(Definition::createDef<MIDIIONode>("MIDI", MIDIIONode::getTypeStringStatic()));
}
