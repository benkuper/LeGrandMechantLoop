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
    updateOutputsFromNode();
}

void AudioInputProcessor::updateOutputsFromNode()
{
    while (inputRMS.size() > nodeRef->numOutputs)
    {
        removeControllable(inputRMS[inputRMS.size()-1]);
        inputRMS.removeLast(1);
    }

    while (inputRMS.size() < nodeRef->numOutputs)
    {
        String s = nodeRef->audioOutputNames[inputRMS.size()];
        FloatParameter* p = addFloatParameter(s, "RMS for this input", 0, 0, 1);
        p->setControllableFeedbackOnly(true);
        inputRMS.add(p);
    }

    setPlayConfigDetails(nodeRef->numOutputs, nodeRef->numOutputs, getSampleRate(), getBlockSize());
}

void AudioInputProcessor::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    for (int i = 0; i < inputRMS.size() && i < buffer.getNumChannels() ; i++)
    {
        float rms = buffer.getRMSLevel(i, 0, buffer.getNumSamples());
        inputRMS[i]->setValue(rms);
    }
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

void AudioOutputProcessor::updateInputsFromNode()
{
    while (outputRMS.size() > nodeRef->audioInputNames.size())
    {
        removeControllable(outputRMS[outputRMS.size() - 1]);
        outputRMS.removeLast(1);
    }

    while (outputRMS.size() < nodeRef->audioInputNames.size())
    {
        FloatParameter* p = addFloatParameter(nodeRef->audioInputNames[outputRMS.size()] + "RMS", "RMS for this output", 0, 0, 1);
        p->setControllableFeedbackOnly(true);
        outputRMS.add(p);
    }
}

void AudioOutputProcessor::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
}

NodeViewUI* AudioOutputProcessor::createNodeViewUI()
{
    return new OutputNodeViewUI((GenericAudioNode<AudioOutputProcessor>*)nodeRef.get());
}