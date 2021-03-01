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
    outItemsCC("Out Master"),
    itemsCC("Items")
{
    viewUISize->setPoint(140, 260);

        
    itemsCC.saveAndLoadRecursiveData = true;
    addChildControllableContainer(&itemsCC);
    itemsCC.editorIsCollapsed = true;

    outItemsCC.saveAndLoadRecursiveData = true;
    addChildControllableContainer(&outItemsCC);
    outItemsCC.editorIsCollapsed = true;
}

void MixerNode::updateAudioInputsInternal()
{
    while (itemsCC.controllableContainers.size() > getNumAudioInputs())
    {
        itemsCC.removeChildControllableContainer(itemsCC.controllableContainers[itemsCC.controllableContainers.size() - 1]);
    }

    while (itemsCC.controllableContainers.size() < getNumAudioInputs())
    {
        ControllableContainer* cc = new ControllableContainer("Input " + String(itemsCC.controllableContainers.size() + 1));
        cc->saveAndLoadRecursiveData = true;
        itemsCC.addChildControllableContainer(cc, true);
        updateGainCC(cc);
    }
}

void MixerNode::updateAudioOutputsInternal()
{
    for (int i = 0; i < itemsCC.controllableContainers.size(); i++) updateGainCC(itemsCC.controllableContainers[i]);


    while (outItemsCC.controllableContainers.size() > getNumAudioOutputs())
    {
        outItemsCC.removeChildControllableContainer(outItemsCC.controllableContainers[outItemsCC.controllableContainers.size() - 1]);
    }

    while (outItemsCC.controllableContainers.size() < getNumAudioOutputs())
    {
        outItemsCC.addChildControllableContainer(new MixerItem(-1, outItemsCC.controllableContainers.size(), true), true);
    }
}


void MixerNode::updateGainCC(ControllableContainer* cc)
{
    while (cc->controllableContainers.size() > getNumAudioOutputs())
    {
        cc->removeChildControllableContainer(cc->controllableContainers[cc->controllableContainers.size() - 1]);
    }

    int inputIndex = itemsCC.controllableContainers.indexOf(cc);

    while (cc->controllableContainers.size() < getNumAudioOutputs())
    {
        cc->addChildControllableContainer(new MixerItem(inputIndex, cc->controllableContainers.size()), true);
    }
}

MixerItem * MixerNode::getMixerItem(int inputIndex, int outputIndex)
{
    return (MixerItem*)itemsCC.controllableContainers[inputIndex]->controllableContainers[outputIndex].get();
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
            MixerItem* mi = getMixerItem(inputIndex, outputIndex);
            float newGain = mi->getGain();
            tmpBuffer.addFromWithRamp(outputIndex, 0, buffer.getReadPointer(inputIndex), numSamples, mi->prevGain, newGain);
            mi->prevGain = newGain;
        }
    }


    for (int outputIndex = 0; outputIndex < numAudioOutputs->intValue(); outputIndex++)
    {
        MixerItem* outMI = (MixerItem*)outItemsCC.controllableContainers[outputIndex].get();
        float newGain = outMI->getGain();
        buffer.clear(outputIndex, 0, numSamples);
        buffer.addFromWithRamp(outputIndex, 0, tmpBuffer.getReadPointer(outputIndex), numSamples, outMI->prevGain, newGain);
        outMI->prevGain = newGain;

        float rms = buffer.getMagnitude(outputIndex, 0, buffer.getNumSamples());
        float curVal = outMI->rms->floatValue();
        float targetVal = outMI->rms->getLerpValueTo(rms, rms > curVal ? .8f : .2f);
        outMI->rms->setValue(targetVal);
    }

}

var MixerNode::getJSONData()
{
    var data = Node::getJSONData();
    data.getDynamicObject()->setProperty("items", itemsCC.getJSONData());
    data.getDynamicObject()->setProperty("outItems", outItemsCC.getJSONData());
    return data;
}

void MixerNode::loadJSONDataItemInternal(var data)
{
    Node::loadJSONDataItemInternal(data);
    autoSetNumAudioInputs();
    autoSetNumAudioOutputs();
    if(data.hasProperty("items")) itemsCC.loadJSONData(data.getProperty("items", var()));
    if(data.hasProperty("outItems")) outItemsCC.loadJSONData(data.getProperty("outItems",var()));
}

BaseNodeViewUI* MixerNode::createViewUI()
{
    return new MixerNodeViewUI(this);
}

MixerItem::MixerItem(int inputIndex, int outputIndex, bool hasRMS) :
    VolumeControl(inputIndex > -1 ? (String(inputIndex + 1) + " > " + String(outputIndex + 1)) : "Out " + String(outputIndex + 1), hasRMS),
    inputIndex(inputIndex),
    outputIndex(outputIndex)
{
}

MixerItem::~MixerItem()
{
}