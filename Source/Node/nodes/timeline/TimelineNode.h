/*
  ==============================================================================

	TimelineNode.h
	Created: 20 Apr 2024 9:00:30pm
	Author:  bkupe

  ==============================================================================
*/

#pragma once

class TimelineNode :
	public Node,
	public Transport::TransportListener
{
public:
	TimelineNode(var params = var());
	~TimelineNode();

	LGMLSequenceManager sequenceManager;
	AudioProcessorGraph containerGraph;
	AudioProcessorGraph::NodeID audioInputID;
	AudioProcessorGraph::NodeID audioOutputID;
	AudioProcessorGraph::NodeID midiInputID;
	AudioProcessorGraph::NodeID midiOutputID;
	AudioProcessorGraph::AudioGraphIOProcessor* audioInNode;
	AudioProcessorGraph::AudioGraphIOProcessor* audioOutNode;
	AudioProcessorGraph::AudioGraphIOProcessor* midiInNode;
	AudioProcessorGraph::AudioGraphIOProcessor* midiOutNode;
	
	void clearItem() override;

	void initInternal() override;
	
	void updateGraph();

	void playStateChanged(bool isPlaying, bool forceRestart) override;

	void updateAudioInputsInternal() override;
	void updateAudioOutputsInternal() override;

	void updatePlayConfigInternal() override;
	void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;


	BaseNodeViewUI* createViewUI() override;

	var getJSONData(bool includeNonOverriden = false) override;
	void loadJSONDataInternal(var data) override;

	

	DECLARE_TYPE("Timeline");
};