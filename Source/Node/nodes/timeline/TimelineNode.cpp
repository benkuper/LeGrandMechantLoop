/*
  ==============================================================================

	TimelineNode.cpp
	Created: 20 Apr 2024 9:00:30pm
	Author:  bkupe

  ==============================================================================
*/

#include "Node/NodeIncludes.h"
#include "TimelineNode.h"

TimelineNode::TimelineNode(var params) :
	Node(getTypeString(), params, true, true, true),
	sequence(this),
	audioInputID(AudioProcessorGraph::NodeID(AudioManager::getInstance()->getNewGraphID())),
	audioOutputID(AudioProcessorGraph::NodeID(AudioManager::getInstance()->getNewGraphID())),
	midiInputID(AudioProcessorGraph::NodeID(AudioManager::getInstance()->getNewGraphID())),
	midiOutputID(AudioProcessorGraph::NodeID(AudioManager::getInstance()->getNewGraphID()))
{
	audioInNode = new AudioProcessorGraph::AudioGraphIOProcessor(AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode);
	audioOutNode = new AudioProcessorGraph::AudioGraphIOProcessor(AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode);
	midiInNode = new AudioProcessorGraph::AudioGraphIOProcessor(AudioProcessorGraph::AudioGraphIOProcessor::midiInputNode);
	midiOutNode = new AudioProcessorGraph::AudioGraphIOProcessor(AudioProcessorGraph::AudioGraphIOProcessor::midiOutputNode);

	containerGraph.addNode(std::unique_ptr< AudioProcessorGraph::AudioGraphIOProcessor>(audioInNode), audioInputID);
	containerGraph.addNode(std::unique_ptr< AudioProcessorGraph::AudioGraphIOProcessor>(audioOutNode), audioOutputID);
	containerGraph.addNode(std::unique_ptr< AudioProcessorGraph::AudioGraphIOProcessor>(midiInNode), midiInputID);
	containerGraph.addNode(std::unique_ptr< AudioProcessorGraph::AudioGraphIOProcessor>(midiOutNode), midiOutputID);

	setMIDIIO(true, true);
	

	addChildControllableContainer(&sequence);

	Transport::getInstance()->addTransportListener(this);
}

TimelineNode::~TimelineNode()
{
	Transport::getInstance()->removeTransportListener(this);
}

void TimelineNode::clearItem()
{
	Node::clearItem();
	sequence.clearItem();
}

void TimelineNode::initInternal()
{
	updateGraph();
}

void TimelineNode::updateGraph()
{
	ScopedSuspender sp(processor);

	audioInNode->setPlayConfigDetails(getNumAudioInputs(), getNumAudioInputs(), processor->getSampleRate(), processor->getBlockSize());
	audioOutNode->setPlayConfigDetails(getNumAudioOutputs(), getNumAudioOutputs(), processor->getSampleRate(), processor->getBlockSize());

	midiInNode->setPlayConfigDetails(getNumAudioInputs(), getNumAudioInputs(), processor->getSampleRate(), processor->getBlockSize());
	midiOutNode->setPlayConfigDetails(getNumAudioOutputs(), getNumAudioOutputs(), processor->getSampleRate(), processor->getBlockSize());

	containerGraph.setPlayConfigDetails(getNumAudioInputs(), getNumAudioOutputs(), processor->getSampleRate(), processor->getBlockSize());
	containerGraph.prepareToPlay(processor->getSampleRate(), processor->getBlockSize());
	
	sequence.updateAudioInputs();
	sequence.updateAudioOutputs();

	 
}

void TimelineNode::playStateChanged(bool isPlaying, bool forceRestart)
{
	Transport::getInstance()->removeTransportListener(this);
}

void TimelineNode::updateAudioInputsInternal()
{
	sequence.updateAudioInputs();
}

void TimelineNode::updateAudioOutputsInternal()
{
	sequence.updateAudioOutputs();
}

void TimelineNode::updatePlayConfigInternal()
{
	Node::updatePlayConfigInternal();
	if (!isCurrentlyLoadingData) updateGraph();
}

void TimelineNode::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
	buffer.clear();
	containerGraph.processBlock(buffer, midiMessages);
}

void TimelineNode::selectThis(bool addToSelection, bool notify)
{
	Node::selectThis(addToSelection, notify);
	if (TimeMachineView* v = ShapeShifterManager::getInstance()->getContentForType<TimeMachineView>()) v->setSequence(&sequence);
}

var TimelineNode::getJSONData()
{
	var data = Node::getJSONData();
	data.getDynamicObject()->setProperty("sequence", sequence.getJSONData());
	return data;
}

void TimelineNode::loadJSONDataInternal(var data)
{
	Node::loadJSONDataItemInternal(data);
	sequence.loadJSONData(data.getProperty("sequence", var()));
	updateGraph();
}
