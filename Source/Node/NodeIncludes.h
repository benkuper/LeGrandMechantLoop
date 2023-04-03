/*
  ==============================================================================

    NodeIncludes.h
    Created: 28 Apr 2021 10:01:54am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"

// Libraries
#ifndef USE_RUBBERBAND
#define USE_RUBBERBAND 1
#endif

#if USE_RUBBERBAND
#include "RubberBandStretcher.h"
#endif

#include "Common/CommonIncludes.h"
#include "Engine/AudioManager.h"
#include "Engine/VSTManager.h"
#include "Transport/Transport.h"
#include "Interface/InterfaceIncludes.h"

#include "Node.h"

#include "Connection/NodeConnection.h"
#include "Connection/NodeConnectionManager.h"

#include "Connection/ui/NodeConnectionEditor.h"
#include "Connection/ui/NodeConnectionViewUI.h"
#include "Connection/ui/NodeConnector.h"
#include "Connection/ui/NodeConnectionManagerViewUI.h"

#include "NodeFactory.h"
#include "NodeManager.h"

#include "nodes/container/ContainerNode.h"

#include "nodes/io/IONode.h"
#include "nodes/io/MIDIIONode.h"

#include "nodes/looper/LooperTrack.h"
#include "nodes/looper/LooperNode.h"

#include "nodes/looper/AudioLooperTrack.h"
#include "nodes/looper/AudioLooperNode.h"

#include "nodes/looper/MIDILooperTrack.h"
#include "nodes/looper/MIDILooperNode.h"


#include "nodes/mixer/MixerNode.h"
#include "nodes/router/AudioRouterNode.h"

#include "nodes/sampler/SamplerNode.h"

#include "nodes/spat/SpatItem.h"
#include "nodes/spat/SpatNode.h"


#include "nodes/vst/VSTLinkedParameter.h"
#include "nodes/vst/VSTNode.h"
#include "nodes/vst/VSTRackNode.h"

//UI
#include "ui/NodeUI.h"
#include "ui/NodeManagerUI.h"
#include "ui/NodeViewUI.h"
#include "ui/NodeManagerViewUI.h"

#include "nodes/io/ui/IONodeViewUI.h"

#include "nodes/mixer/ui/MixerNodeViewUI.h"

#include "nodes/looper/ui/LooperTrackUI.h"
#include "nodes/looper/ui/LooperNodeViewUI.h"

#include "nodes/router/ui/AudioRouterNodeUI.h"

#include "nodes/sampler/ui/SamplerNodeUI.h"

#include "nodes/spat/ui/SpatItemUI.h"
#include "nodes/spat/ui/SpatView.h"
#include "nodes/spat/ui/SpatNodeViewUI.h"

#include "nodes/recorder/RecorderNode.h"

#include "nodes/analysis/libs/pitch/PitchDetector.h"
#include "nodes/analysis/libs/pitch/PitchMPM.h"
#include "nodes/analysis/libs/pitch/PitchYIN.h"

#include "nodes/analysis/FFTAnalyzer.h"
#include "nodes/analysis/FFTAnalyzerManager.h"
#include "nodes/analysis/ui/FFTAnalyzerEditor.h"
#include "nodes/analysis/ui/FFTAnalyzerManagerEditor.h"
#include "nodes/analysis/AnalysisNode.h"

#include "nodes/container/ui/ContainerNodeUI.h"

#include "nodes/vst/ui/VSTLinkedParameterUI.h"
#include "nodes/vst/ui/VSTNodeViewUI.h"
#include "nodes/vst/ui/VSTRackNodeUI.h"
