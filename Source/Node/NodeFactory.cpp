/*
  ==============================================================================

	NodeFactory.cpp
	Created: 15 Nov 2020 8:56:42am
	Author:  bkupe

  ==============================================================================
*/

#include "Node/NodeIncludes.h"

juce_ImplementSingleton(NodeFactory)

NodeFactory::NodeFactory()
{
	defs.add(Definition::createDef<ContainerNode>(""));

	defs.add(Definition::createDef<AudioInputNode>("Audio"));
	defs.add(Definition::createDef<AudioOutputNode>("Audio"));
	defs.add(Definition::createDef<AudioLooperNode>("Audio"));
	defs.add(Definition::createDef<SamplerNode>("Audio"));
	defs.add(Definition::createDef<VSTNode>("Audio"));
	defs.add(Definition::createDef<MixerNode>("Audio"));
	defs.add(Definition::createDef<SpatNode>("Audio"));
	defs.add(Definition::createDef<AudioRouterNode>("Audio"));
	defs.add(Definition::createDef<RecorderNode>("Audio"));
	defs.add(Definition::createDef<AnalysisNode>("Audio"));
	defs.add(Definition::createDef<MetronomeNode>("Audio"));

	defs.add(Definition::createDef<MIDILooperNode>("MIDI"));
	defs.add(Definition::createDef<MIDIInputNode>("MIDI"));
	defs.add(Definition::createDef<MIDIOutputNode>("MIDI"));

	defs.add(Definition::createDef<TimelineNode>("General"));

}
