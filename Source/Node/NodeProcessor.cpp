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

NodeProcessor::NodeProcessor(Node* n, bool hasAudioInput, bool hasAudioOutput, bool useOutRMS) :
    ControllableContainer("Processor"),
    nodeRef(n),
    hasAudioInput(hasAudioInput),
    hasAudioOutput(hasAudioOutput),
    outRMS(nullptr)
{
    hideEditorHeader = true;

    if (useOutRMS)
    {
        outRMS = addFloatParameter("Out RMS", "The general activity of all channels combined after processing", 0, 0, 1);
        outRMS->setControllableFeedbackOnly(true);
    }
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

    if (outRMS != nullptr)
    {
        float rms = 0;
        for (int i = 0; i < buffer.getNumChannels(); i++) rms = jmax(rms, buffer.getRMSLevel(i, 0, buffer.getNumSamples()));
        float curVal = outRMS->floatValue();
        float targetVal = outRMS->getLerpValueTo(rms, rms > curVal ? .8f : .2f);
        outRMS->setValue(targetVal);
    }
}

GenericNodeProcessor::GenericNodeProcessor(Node* n, bool hasInput, bool hasOutput, bool useOutRMS) :
    NodeProcessor(n, hasInput, hasOutput, useOutRMS)
{
}

NodeViewUI* GenericNodeProcessor::createNodeViewUI()
{
    return new NodeViewUI(nodeRef.get()); 
}
