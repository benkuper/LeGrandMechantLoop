/*
  ==============================================================================

    NodeProcessor.cpp
    Created: 15 Nov 2020 10:34:19pm
    Author:  bkupe

  ==============================================================================
*/

#include "NodeProcessor.h"
#include "Node.h"
#include "ui/NodeViewUI.h"
#include "Engine/AudioManager.h"
#include "Connection/NodeConnection.h"
#include "Transport/Transport.h"

NodeProcessor::NodeProcessor(Node* n, bool hasAudioInput, bool hasAudioOutput, bool userCanSetIO, bool useOutControl) :
    ControllableContainer("Processor"),
    nodeRef(n),
    hasAudioInput(hasAudioInput),
    hasAudioOutput(hasAudioOutput),
    numAudioInputs(nullptr),
    numAudioOutputs(nullptr),
    outGain(nullptr),
    outRMS(nullptr)
{
    hideEditorHeader = true;

    if (userCanSetIO)
    {
        if (hasAudioInput)
        {
            numAudioInputs = addIntParameter("Audio Inputs", "Number of audio inputs for this node", 2, 0, 32);
            nodeRef->setAudioInputs(numAudioInputs->intValue());
        }
        
        if (hasAudioOutput)
        {
            numAudioOutputs = addIntParameter("Audio Outputs", "Number of audio outputs for this node", 2, 0, 32);
            nodeRef->setAudioOutputs(numAudioOutputs->intValue());
        }
    }

    if (useOutControl)
    {
        outGain = addFloatParameter("Out Gain", "Gain to apply to all channels after processing", 1, 0, 2);
        outRMS = addFloatParameter("Out RMS", "The general activity of all channels combined after processing", 0, 0, 1);
        outRMS->setControllableFeedbackOnly(true);
    }

}

void NodeProcessor::init()
{
    setPlayHead(Transport::getInstance()); //is it interesting ? or should each node take care if it if necessary ?


    updateInputsFromNode(false);
    updateOutputsFromNode(false);
    initInternal();
    updatePlayConfig();
}

void NodeProcessor::onContainerParameterChanged(Parameter* p)
{
    if (p == numAudioInputs) autoSetNumInputs();
    else if (p == numAudioOutputs) autoSetNumOutputs();
}

void NodeProcessor::autoSetNumInputs()
{
    nodeRef->setAudioInputs(numAudioInputs->intValue());
}

void NodeProcessor::autoSetNumOutputs()
{
    nodeRef->setAudioOutputs(numAudioInputs->intValue());
}

void NodeProcessor::updateInputsFromNode(bool _updatePlayConfig)
{
    bool shouldResume = !isSuspended();
    suspendProcessing(true);

    updateInputsFromNodeInternal();
    if(_updatePlayConfig) updatePlayConfig();

    if(shouldResume) suspendProcessing(false);
}

void NodeProcessor::updateOutputsFromNode(bool _updatePlayConfig)
{
    bool shouldResume = !isSuspended();
    suspendProcessing(true);
    connectionsActivityLevels.resize(nodeRef->numOutputs);
    updateOutputsFromNodeInternal();
    if (_updatePlayConfig) updatePlayConfig();

    if(shouldResume) suspendProcessing(false);
}

/* AudioProcessor */
void NodeProcessor::updatePlayConfig()
{
    setPlayConfigDetails(nodeRef->numInputs, nodeRef->numOutputs, getSampleRate(), getBlockSize());
    if (!isCurrentlyLoadingData)
    {
        nodeRef->notifyPlayConfigUpdated();
    }
    //NLOG(nodeRef->niceName, "Buffer has now " << getChannel)

}

void NodeProcessor::receiveMIDIFromInput(Node* n, MidiBuffer& inputBuffer)
{
    inMidiBuffer.addEvents(inputBuffer, 0, getBlockSize(), 0);
}

const String NodeProcessor::getName() const {
    return nodeRef.wasObjectDeleted() ? "[Deleted]" : nodeRef->niceName; 
}

void NodeProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    if (nodeRef == nullptr || nodeRef.wasObjectDeleted())
    {
        buffer.clear();
        return;
    }

    if (!nodeRef->enabled->boolValue())
    {
        processBlockBypassed(buffer, midiMessages);
        return;
    }

    if (buffer.getNumChannels() < jmax(nodeRef->numInputs, nodeRef->numOutputs))
    {
        LOGWARNING("Not the same number of channels ! ");
        return;
    }

    processBlockInternal(buffer, inMidiBuffer);

    if (outGain != nullptr)
    {
        buffer.applyGain(outGain->floatValue());
    }

    float rms = 0;
    
    connectionsActivityLevels.fill(0);
    int numOutConnections = nodeRef->outAudioConnections.size();

    for (int i = 0; i < buffer.getNumChannels(); i++)
    {
        float channelRMS = buffer.getRMSLevel(i, 0, buffer.getNumSamples());
        rms = jmax(rms, channelRMS);
        
        for (int c = 0; c < numOutConnections; c++)
        {
            for (auto& cm : nodeRef->outAudioConnections[c]->channelMap)
            {
                if (cm.sourceChannel == i) connectionsActivityLevels.set(c, jmax(connectionsActivityLevels[c], channelRMS));
            }
        }
    }

    for (int i = 0; i < nodeRef->outAudioConnections.size(); i++)
    {
        float curLevel = nodeRef->outAudioConnections[i]->activityLevel;
        float level = connectionsActivityLevels[i];
        nodeRef->outAudioConnections[i]->activityLevel = curLevel + (level - curLevel) * .2f;
    }

    if (outRMS != nullptr)
    {
        float curVal = outRMS->floatValue();
        float targetVal = outRMS->getLerpValueTo(rms, rms > curVal ? .8f : .2f);
        outRMS->setValue(targetVal);
    }
}

GenericNodeProcessor::GenericNodeProcessor(Node* n, bool hasInput, bool hasOutput, bool userCanSetIO, bool useOutControl) :
    NodeProcessor(n, hasInput, hasOutput, userCanSetIO, useOutControl)
{
}

NodeViewUI* GenericNodeProcessor::createNodeViewUI()
{
    return new NodeViewUI(nodeRef.get()); 
}
