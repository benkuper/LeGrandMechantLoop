/*
  ==============================================================================

	TimelineNode.cpp
	Created: 20 Apr 2024 9:00:30pm
	Author:  bkupe

  ==============================================================================
*/

#include "Node/NodeIncludes.h"

TimelineNode::TimelineNode(var params) :
	Node(getTypeString(), params, true, true, true),
	sequenceManager(this),
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
	
	addChildControllableContainer(&sequenceManager);
	if (!Engine::mainEngine->isLoadingFile) sequenceManager.addItem();

	Transport::getInstance()->addTransportListener(this);
}

TimelineNode::~TimelineNode()
{
	Transport::getInstance()->removeTransportListener(this);
}

void TimelineNode::clearItem()
{
	sequenceManager.clear();
	Node::clearItem();
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
	
	for (auto& i : sequenceManager.items)
	{
		LGMLSequence* s = dynamic_cast<LGMLSequence*>(i);
		s->updateAudioInputs();
		s->updateAudioOutputs();
	}
		 
}

void TimelineNode::playStateChanged(bool isPlaying, bool forceRestart)
{
	Transport::getInstance()->removeTransportListener(this);
}

void TimelineNode::updateAudioInputsInternal()
{
	for (auto& i : sequenceManager.items)
	{
		LGMLSequence* s = dynamic_cast<LGMLSequence*>(i);
		s->updateAudioInputs();
	}
}

void TimelineNode::updateAudioOutputsInternal()
{
	for (auto& i : sequenceManager.items)
	{
		LGMLSequence* s = dynamic_cast<LGMLSequence*>(i);
		s->updateAudioOutputs();
	}
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


BaseNodeViewUI* TimelineNode::createViewUI()
{
	return new TimelineNodeUI(this);
}

var TimelineNode::getJSONData(bool includeNonOverriden)
{
	var data = Node::getJSONData(includeNonOverriden);
	data.getDynamicObject()->setProperty(sequenceManager.shortName, sequenceManager.getJSONData());
	return data;
}

void TimelineNode::loadJSONDataInternal(var data)
{
	Node::loadJSONDataItemInternal(data);
	sequenceManager.loadJSONData(data.getProperty(sequenceManager.shortName, var()));
	updateGraph();
}
