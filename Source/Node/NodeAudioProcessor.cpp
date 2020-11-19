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
#include "Engine/AudioManager.h"

NodeAudioProcessor::NodeAudioProcessor(Node* n, bool hasInput, bool hasOutput, bool useOutRMS) : 
    ControllableContainer("Processor"),
    nodeRef(n),
    hasInput(hasInput),
    hasOutput(hasOutput),
    outRMS(nullptr)
{
    hideEditorHeader = true;

    if (useOutRMS)
    {
        outRMS = addFloatParameter("Out RMS", "The general activity of all channels combined after processing", 0, 0, 1);
        outRMS->setControllableFeedbackOnly(true);
    }
}


void NodeAudioProcessor::updateInputsFromNode(bool _updatePlayConfig)
{
    suspendProcessing(true);
    GenericScopedLock<SpinLock> lock(processorLock);

    updateInputsFromNodeInternal();
    if(_updatePlayConfig) updatePlayConfig();

    suspendProcessing(false);
}

void NodeAudioProcessor::updateOutputsFromNode(bool _updatePlayConfig)
{
    suspendProcessing(true);
    GenericScopedLock<SpinLock> lock(processorLock);

    updateOutputsFromNodeInternal();
    if (_updatePlayConfig) updatePlayConfig();

    suspendProcessing(false);

}

/* AudioProcessor */
void NodeAudioProcessor::updatePlayConfig()
{
    setPlayConfigDetails(nodeRef->numInputs, nodeRef->numOutputs, getSampleRate(), getBlockSize());
    if (!isCurrentlyLoadingData) AudioManager::getInstance()->updateGraph();
    //NLOG(nodeRef->niceName, "Buffer has now " << getChannel)
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

    if (buffer.getNumChannels() < jmax(nodeRef->numInputs, nodeRef->numOutputs))
    {
        LOGWARNING("Not the same number of channels ! ");
        return;
    }

    {
        GenericScopedLock<SpinLock> lock(processorLock);

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
}

GenericNodeAudioProcessor::GenericNodeAudioProcessor(Node* n, bool hasInput, bool hasOutput, bool useOutRMS) :
    NodeAudioProcessor(n, hasInput, hasOutput, useOutRMS)
{
}

NodeViewUI* GenericNodeAudioProcessor::createNodeViewUI()
{
    return new NodeViewUI(nodeRef.get()); 
}
