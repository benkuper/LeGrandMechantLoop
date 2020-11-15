/*
  ==============================================================================

    IONode.cpp
    Created: 15 Nov 2020 8:42:54am
    Author:  bkupe

  ==============================================================================
*/

#include "IONode.h"

IONode::IONode(StringRef name, Type type, NodeAudioProcessor * processor, var params) :
    Node(name, INPUT, processor, params)
{
}

IONode::~IONode()
{
}

AudioInputNode::AudioInputNode(var params) :
    IONode(getTypeString(), INPUT, new InputProcessor(this), params)
{

}

AudioInputNode::~AudioInputNode()
{
}

AudioOutputNode::AudioOutputNode(var params) :
    IONode(getTypeString(), OUTPUT, new OutputProcessor(this), params)
{
}

AudioOutputNode::~AudioOutputNode()
{
}

AudioInputNode::InputProcessor::InputProcessor(AudioInputNode* node) :
    NodeAudioProcessor(node),
    inputNode(node)
{
}

AudioInputNode::InputProcessor::~InputProcessor()
{
}

void AudioInputNode::InputProcessor::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
}

AudioOutputNode::OutputProcessor::OutputProcessor(AudioOutputNode* node) :
    NodeAudioProcessor(node),
    outputNode(node)
{
}

AudioOutputNode::OutputProcessor::~OutputProcessor()
{
}

void AudioOutputNode::OutputProcessor::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
}
