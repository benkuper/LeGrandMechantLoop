/*
  ==============================================================================

    NodeIncludes.cpp
    Created: 28 Apr 2021 10:01:54am
    Author:  bkupe

  ==============================================================================
*/

#include "NodeIncludes.h"
#include "Preset/PresetIncludes.h"

#include "Node.cpp"

#include "Connection/NodeConnection.cpp"
#include "Connection/NodeConnectionManager.cpp"

#include "NodeFactory.cpp"
#include "NodeManager.cpp"

#include "nodes/container/ContainerNode.cpp"
#include "nodes/container/ui/ContainerNodeUI.cpp"

#include "nodes/io/IONode.cpp"
#include "nodes/io/MIDIIONode.cpp"
#include "nodes/io/ui/IONodeViewUI.cpp"

#include "nodes/looper/LooperTrack.cpp"
#include "nodes/looper/LooperNode.cpp"

#include "nodes/looper/AudioLooperTrack.cpp"
#include "nodes/looper/AudioLooperNode.cpp"

#include "nodes/looper/MIDILooperTrack.cpp"
#include "nodes/looper/MIDILooperNode.cpp"

#include "nodes/looper/ui/LooperTrackUI.cpp"
#include "nodes/looper/ui/LooperNodeViewUI.cpp"

#include "nodes/mixer/MixerNode.cpp"
#include "nodes/mixer/ui/MixerNodeViewUI.cpp"

#include "nodes/router/AudioRouterNode.cpp"
#include "nodes/router/ui/AudioRouterNodeUI.cpp"

#include "nodes/sampler/SamplerNode.cpp"
#include "nodes/sampler/ui/SamplerNodeUI.cpp"

#include "nodes/spat/SpatItem.cpp"
#include "nodes/spat/SpatNode.cpp"


#include "nodes/spat/ui/SpatItemUI.cpp"
#include "nodes/spat/ui/SpatNodeViewUI.cpp"
#include "nodes/spat/ui/SpatView.cpp"

#include "nodes/vst/VSTLinkedParameter.cpp"
#include "nodes/vst/VSTNode.cpp"
#include "nodes/vst/VSTRackNode.cpp"

#include "nodes/recorder/RecorderNode.cpp"

#include "nodes/analysis/FFTAnalyzer.cpp"
#include "nodes/analysis/FFTAnalyzerManager.cpp"
#include "nodes/analysis/ui/FFTAnalyzerEditor.cpp"
#include "nodes/analysis/ui/FFTAnalyzerManagerEditor.cpp"
#include "nodes/analysis/AnalysisNode.cpp"

#include "nodes/vst/ui/VSTLinkedParameterUI.cpp"
#include "nodes/vst/ui/VSTNodeViewUI.cpp"
#include "nodes/vst/ui/VSTRackNodeUI.cpp"


#include "ui/NodeUI.cpp"
#include "ui/NodeManagerUI.cpp"
#include "ui/NodeViewUI.cpp"
#include "ui/NodeManagerViewUI.cpp"

#include "Connection/ui/NodeConnectionEditor.cpp"
#include "Connection/ui/NodeConnectionViewUI.cpp"
#include "Connection/ui/NodeConnector.cpp"
#include "Connection/ui/NodeConnectionManagerViewUI.cpp"
