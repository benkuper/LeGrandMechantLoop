/*
  ==============================================================================

    AudioRouterNode.cpp
    Created: 4 Dec 2020 11:55:34am
    Author:  bkupe

  ==============================================================================
*/

#include "AudioRouterNode.h"
#include "ui/AudioRouterNodeUI.h"

AudioRouterProcessor::AudioRouterProcessor(Node * n) :
    GenericNodeProcessor(n, true, true, true, true),
    timeAtTransition(0)
{
    numChannelsPerGroup = addIntParameter("Channels Per Group", "Each input and output is a group of that many channels", 1, 1, 32);
    currentInput = addIntParameter("Input", "The current input to choose.", 1, 1, numAudioInputs->intValue());
    currentOutput = addIntParameter("Output", "The current input to choose.", 1, 1, numAudioOutputs->intValue());
    transitionMS = addIntParameter("Transition", "The time to transition from one output to another. This allows for smooth transition", 100, 0, 1000);

    previousInput = currentInput->intValue();
    previousOutput = currentOutput->intValue();
}

AudioRouterProcessor::~AudioRouterProcessor()
{
}

void AudioRouterProcessor::autoSetNumInputs()
{
    StringArray inputNames;
    for (int i = 0; i < numAudioInputs->intValue(); i++)
    {
        for (int j = 0; j < numChannelsPerGroup->intValue(); j++)
        {
            inputNames.add("Input "+String(i + 1) + ":" + String(j + 1));
        }
    }
    nodeRef->setAudioInputs(inputNames);
}

void AudioRouterProcessor::autoSetNumOutputs()
{
    StringArray outputNames;

    for (int i = 0; i < numAudioOutputs->intValue(); i++)
    {
        for (int j = 0; j < numChannelsPerGroup->intValue(); j++)
        {
            outputNames.add("Output "+String(i + 1) + ":" + String(j + 1));
        }
    }

    nodeRef->setAudioOutputs(outputNames);
}

void AudioRouterProcessor::updateInputsFromNodeInternal()
{
    currentInput->setRange(1, numAudioInputs->intValue());
}

void AudioRouterProcessor::updateOutputsFromNodeInternal()
{
    currentOutput->setRange(1, numAudioOutputs->intValue());
}

void AudioRouterProcessor::onContainerParameterChanged(Parameter* p)
{
    GenericNodeProcessor::onContainerParameterChanged(p);
    if (p == numChannelsPerGroup)
    {
        autoSetNumInputs();
        autoSetNumOutputs();
    }
    else if (p == currentInput || p == currentOutput)
    {
        timeAtTransition = Time::getMillisecondCounter();
    }
}

void AudioRouterProcessor::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    int channelsPerGroup = numChannelsPerGroup->intValue();
    int startInputChannel = (currentInput->intValue()-1) * channelsPerGroup;
    int startOutputChannel = (currentOutput->intValue()-1) * channelsPerGroup;

    uint32 t = Time::getMillisecondCounter();
    bool isTransitionning = t < timeAtTransition + transitionMS->intValue();
    
    if (isTransitionning)
    {

        float weight = jlimit<float>(0, 1, (t - timeAtTransition) * 1.0 / transitionMS->intValue());
        int prevStartInputChannel = (previousInput - 1) * channelsPerGroup;
        int prevStartOutputChannel = (previousOutput - 1) * channelsPerGroup;

        tmpBuffer.makeCopyOf(buffer);
        buffer.clear();

        for (int i = 0; i < channelsPerGroup; i++)
        {
            int sourceChannel = startInputChannel + i;
            int destChannel = startOutputChannel + i;
            buffer.addFrom(destChannel, 0, tmpBuffer, sourceChannel, 0, tmpBuffer.getNumSamples(), weight);

            int prevSourceChannel = prevStartInputChannel + i;
            int prevDestChannel = prevStartOutputChannel + i;
            buffer.addFrom(prevDestChannel, 0, tmpBuffer, prevSourceChannel, 0, tmpBuffer.getNumSamples(), 1-weight);
        }
    }
    else
    {
        previousInput = currentInput->intValue();
        previousOutput = currentOutput->intValue();

        //Copy inputs to outputs
        for (int i = 0; i < channelsPerGroup; i++)
        {
            int sourceChannel = startInputChannel + i;
            int destChannel = startOutputChannel + i;
            if (sourceChannel != destChannel) buffer.copyFrom(destChannel, 0, buffer, sourceChannel, 0, buffer.getNumSamples());
        }

        //Clear all the unused channels
        for (int i = 0; i < buffer.getNumChannels(); i++)
        {
            if (i < startOutputChannel || i >= startOutputChannel + channelsPerGroup) buffer.clear(i, 0, buffer.getNumSamples());
        }
    }
    

    
}

NodeViewUI* AudioRouterProcessor::createNodeViewUI()
{
    return new AudioRouterNodeViewUI((GenericNode<AudioRouterProcessor> *)nodeRef.get());
}
