/*
  ==============================================================================

    IONode.cpp
    Created: 15 Nov 2020 8:42:54am
    Author:  bkupe

  ==============================================================================
*/

#include "IONode.h"
#include "ui/IONodeViewUI.h"

// INPUT
AudioInputProcessor::AudioInputProcessor(Node* node) :
    IOProcessor(node)
{
}

void AudioInputProcessor::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
}

NodeViewUI* AudioInputProcessor::createNodeViewUI()
{
    return new InputNodeViewUI((GenericAudioNode<AudioInputProcessor> *)nodeRef.get());
}

//OUTPUT
AudioOutputProcessor::AudioOutputProcessor(Node* node) :
    IOProcessor(node)
{
}

void AudioOutputProcessor::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
}

NodeViewUI* AudioOutputProcessor::createNodeViewUI()
{
    return new OutputNodeViewUI((GenericAudioNode<AudioOutputProcessor>*)nodeRef.get());
}