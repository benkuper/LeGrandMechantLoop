/*
  ==============================================================================

    IONode.cpp
    Created: 15 Nov 2020 8:42:54am
    Author:  bkupe

  ==============================================================================
*/

#include "IONode.h"
#include "ui/IONodeViewUI.h"

IOProcessor::IOProcessor(Node* n, bool isInput) :
    GenericNodeAudioProcessor(n),
    rmsCC("RMS"),
    gainCC("Gain"),
    isInput(isInput)
{
    addChildControllableContainer(&rmsCC);
    addChildControllableContainer(&gainCC);

    rmsCC.editorIsCollapsed = true;

    updateIOFromNode();
}

void IOProcessor::updateInputsFromNode()
{
    if (!isInput) updateIOFromNode();
}

void IOProcessor::updateOutputsFromNode()
{
    if (isInput) updateIOFromNode();
}

void IOProcessor::updateIOFromNode()
{
    int numChannels = isInput ? nodeRef->numOutputs : nodeRef->numInputs;
    StringArray names = isInput ? nodeRef->audioOutputNames : nodeRef->audioInputNames;

    //remove surplus
    while (rmsCC.controllables.size() > numChannels)
    {
        rmsCC.removeControllable(rmsCC.controllables[rmsCC.controllables.size() - 1]);
        gainCC.removeControllable(gainCC.controllables[gainCC.controllables.size() - 1]);
    }

    //rename existing
    for (int i = 0; i < gainCC.controllables.size(); i++)
    {
        String s = names[i];
        gainCC.controllables[i]->setNiceName(s);
    }

    //add more
    while (rmsCC.controllables.size() < numChannels)
    {
        String s = names[gainCC.controllables.size()];

        FloatParameter* p = rmsCC.addFloatParameter(s, "RMS for this input", 0, 0, 1);
        p->setControllableFeedbackOnly(true);

        gainCC.addFloatParameter(s, "Gain for this input", 1, 0, 2);
    }

    setPlayConfigDetails(numChannels, numChannels, getSampleRate(), getBlockSize());
}

void IOProcessor::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    for (int i = 0; i < rmsCC.controllables.size() && i < buffer.getNumChannels(); i++)
    {
        FloatParameter* rmsP = (FloatParameter*)rmsCC.controllables[i];
        FloatParameter* gainP = (FloatParameter*)gainCC.controllables[i];

        if (rmsP == nullptr || gainP == nullptr) return;

        buffer.applyGain(i, 0, buffer.getNumSamples(), gainP->floatValue());

        float rms = buffer.getRMSLevel(i, 0, buffer.getNumSamples());
        float curVal = rmsP->floatValue();
        float targetVal = rmsP->getLerpValueTo(rms, rms > curVal ? .8f : .2f);
        rmsP->setValue(targetVal);
    }
}

NodeViewUI* IOProcessor::createNodeViewUI()
{
    return new IONodeViewUI((GenericAudioNode<IOProcessor>*)nodeRef.get());
}
