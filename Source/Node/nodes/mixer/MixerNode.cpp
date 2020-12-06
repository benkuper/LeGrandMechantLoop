/*
  ==============================================================================

    MixerNode.cpp
    Created: 15 Nov 2020 8:42:42am
    Author:  bkupe

  ==============================================================================
*/

#include "MixerNode.h"
#include "ui/MixerNodeViewUI.h"

MixerNode::MixerNode(var params) :
    Node(getTypeString(), params, true, true, true, true),
    rmsCC("RMS"),
    outGainsCC("Out Gains"),
    gainRootCC("Channel Gains")
{
    viewUISize->setPoint(100, 250);

    addChildControllableContainer(&rmsCC);
    rmsCC.editorIsCollapsed = true;
    addChildControllableContainer(&outGainsCC);
    outGainsCC.editorIsCollapsed = true;
    addChildControllableContainer(&gainRootCC);
}

void MixerNode::updateAudioInputsInternal()
{
    while (gainRootCC.controllableContainers.size() > getNumAudioInputs())
    {
        gainRootCC.removeChildControllableContainer(gainRootCC.controllableContainers[gainRootCC.controllableContainers.size() - 1]);
    }

    while (gainRootCC.controllableContainers.size() < getNumAudioInputs())
    {
        addGainCC();
    }
}

void MixerNode::updateAudioOutputsInternal()
{
    for (int i = 0; i < gainRootCC.controllableContainers.size(); i++) updateGainCC(gainRootCC.controllableContainers[i]);


    while (rmsCC.controllables.size() > getNumAudioOutputs())
    {
        rmsCC.removeControllable(rmsCC.controllables[rmsCC.controllables.size() - 1]);
        outGainsCC.removeControllable(outGainsCC.controllables[outGainsCC.controllables.size() - 1]);
    }

    while (rmsCC.controllables.size() < getNumAudioOutputs())
    {
        String oIndex = String(rmsCC.controllables.size() + 1);
        FloatParameter* rmsP = rmsCC.addFloatParameter("RMS "+oIndex, "RMS for output " + oIndex, 0, 0, 1);
        rmsP->setControllableFeedbackOnly(true);

       outGainsCC.addFloatParameter("Gain " + oIndex, "RMS for output " + oIndex, 1, 0, 2);
    }
}

void MixerNode::addGainCC()
{
    ControllableContainer* cc = new ControllableContainer("Input " + String(gainRootCC.controllableContainers.size() + 1));
    gainRootCC.addChildControllableContainer(cc, true);

    updateGainCC(cc);
}

void MixerNode::updateGainCC(ControllableContainer* cc)
{
    while (cc->controllables.size() > getNumAudioOutputs())
    {
        cc->removeControllable(cc->controllables[cc->controllables.size() - 1]);
    }

    int inputIndex = gainRootCC.controllableContainers.indexOf(cc);

    while (cc->controllables.size() < getNumAudioOutputs())
    {
        String i = String(inputIndex + 1);
        String o = String(cc->controllables.size() + 1);
        cc->addFloatParameter(i + " > " + o, "Gain to apply for input " + i + " to output " + o, 1, 0, 2);
    }
}

FloatParameter * MixerNode::getGainParameter(int inputIndex, int outputIndex)
{
    return (FloatParameter*)gainRootCC.controllableContainers[inputIndex]->controllables[outputIndex];
}

void MixerNode::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    int numSamples = buffer.getNumSamples();
    tmpBuffer.setSize(numAudioOutputs->intValue(), numSamples);
    tmpBuffer.clear();

    for (int inputIndex = 0; inputIndex < numAudioInputs->intValue(); inputIndex++)
    {
        for (int outputIndex = 0;  outputIndex < numAudioOutputs->intValue(); outputIndex++)
        {
            FloatParameter* gp = getGainParameter(inputIndex, outputIndex);
            tmpBuffer.addFrom(outputIndex, 0, buffer.getReadPointer(inputIndex), numSamples, gp->floatValue());
        }
    }


    for (int outputIndex = 0; outputIndex < numAudioOutputs->intValue(); outputIndex++)
    {
        FloatParameter* outGainP = (FloatParameter*)outGainsCC.controllables[outputIndex];

        buffer.clear(outputIndex, 0, numSamples);
        buffer.addFrom(outputIndex, 0, tmpBuffer.getReadPointer(outputIndex), numSamples, outGainP->floatValue());

        FloatParameter* rp = (FloatParameter*)rmsCC.controllables[outputIndex];
        float rms = buffer.getRMSLevel(outputIndex, 0, buffer.getNumSamples());
        float curVal = rp->floatValue();
        float targetVal = rp->getLerpValueTo(rms, rms > curVal ? .8f : .2f);
        rp->setValue(targetVal);
    }

}

BaseNodeViewUI* MixerNode::createViewUI()
{
    return new MixerNodeViewUI(this);
}
