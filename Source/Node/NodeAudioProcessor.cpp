/*
  ==============================================================================

    NodeAudioProcessor.cpp
    Created: 15 Nov 2020 10:34:19pm
    Author:  bkupe

  ==============================================================================
*/

#include "NodeAudioProcessor.h"
#include "Node.h"
#include "ui/NodeViewUI.h"

NodeAudioProcessor::NodeAudioProcessor(Node* n, bool useOutRMS) : 
    ControllableContainer("Processor"),
    nodeRef(n),
    outRMS(nullptr)
{
    hideEditorHeader = true;

    if (useOutRMS)
    {
        outRMS = addFloatParameter("Out RMS", "The general activity of all channels combined after processing", 0, 0, 1);
        outRMS->setControllableFeedbackOnly(true);
    }
}


void NodeAudioProcessor::updateInputsFromNode()
{
    updateInputsFromNodeInternal();
    updatePlayConfig();
}

void NodeAudioProcessor::updateOutputsFromNode()
{
    updateOutputsFromNodeInternal();
    updatePlayConfig();
}

int NodeAudioProcessor::getExpectedNumInputs() {
    return nodeRef->numInputs; 
}

int NodeAudioProcessor::getExpectedNumOutputs() {
    return nodeRef->numOutputs; 
}

/* AudioProcessor */
void NodeAudioProcessor::updatePlayConfig()
{
    setPlayConfigDetails(getExpectedNumInputs(), getExpectedNumOutputs(), getSampleRate(), getBlockSize());
}

const String NodeAudioProcessor::getName() const {
    return nodeRef.wasObjectDeleted() ? "[Deleted]" : nodeRef->niceName; 
}

void NodeAudioProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
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

    processBlockInternal(buffer, midiMessages);

    if (outRMS != nullptr)
    {
        float rms = 0;
        for (int i = 0; i < buffer.getNumChannels(); i++) rms = jmax(rms, buffer.getRMSLevel(i, 0, buffer.getNumSamples()));
        float curVal = outRMS->floatValue();
        float targetVal = outRMS->getLerpValueTo(rms, rms > curVal ? .8f : .2f);
        outRMS->setValue(targetVal);
    }
}

GenericNodeAudioProcessor::GenericNodeAudioProcessor(Node* n, bool useOutRMS) :
    NodeAudioProcessor(n, useOutRMS)
{
}

NodeViewUI* GenericNodeAudioProcessor::createNodeViewUI()
{
    return new NodeViewUI(nodeRef.get()); 
}
