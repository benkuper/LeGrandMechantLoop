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
	itemsCC("Items")
{
	viewUISize->setPoint(200, 300);

	showOutputGains = viewCC.addBoolParameter("Show Outputs Gain", "Show Output Gain", true);
	showOutputRMS = viewCC.addBoolParameter("Show Outputs RMS", "Show Output RMS", true);
	showOutputActives = viewCC.addBoolParameter("Show Outputs Active", "Show Output Active", true);
	showOutputExclusives = viewCC.addBoolParameter("Show Outputs Exclusives", "Show Output Exclusives", true);
	showItemGains = viewCC.addBoolParameter("Show Items Gain", "Show Items Gain", true);
	showItemActives = viewCC.addBoolParameter("Show Items Active", "Show Items Active", true);

	itemsCC.saveAndLoadRecursiveData = true;
	addChildControllableContainer(&itemsCC);
	itemsCC.editorIsCollapsed = true;
}

void MixerNode::updateAudioInputsInternal()
{
	for (int i = 0; i < itemsCC.controllableContainers.size(); i++) ((OutputLineCC*)itemsCC.controllableContainers[i].get())->setInputNumber(getNumAudioInputs());
}

void MixerNode::updateAudioOutputsInternal()
{
	while (outputLines.size() > getNumAudioOutputs())
	{
		OutputLineCC* cc = outputLines[outputLines.size() - 1];
		outputLines.removeAllInstancesOf(cc);
		itemsCC.removeChildControllableContainer(cc);
	}

	while (outputLines.size() < getNumAudioOutputs())
	{
		OutputLineCC* cc = new OutputLineCC(outputLines.size());
		cc->setInputNumber(getNumAudioInputs());
		itemsCC.addChildControllableContainer(cc, true);
		outputLines.add(cc);
	}
}

MixerItem* MixerNode::getMixerItem(int inputIndex, int outputIndex)
{
	return  outputLines[outputIndex]->mixerItems[inputIndex];
}

void MixerNode::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
	int numSamples = buffer.getNumSamples();
	tmpBuffer.setSize(numAudioOutputs->intValue(), numSamples);
	tmpBuffer.clear();

	for (int inputIndex = 0; inputIndex < numAudioInputs->intValue(); inputIndex++)
	{
		for (int outputIndex = 0; outputIndex < numAudioOutputs->intValue(); outputIndex++)
		{
			MixerItem* mi = getMixerItem(inputIndex, outputIndex);
			float newGain = mi->getGain();
			tmpBuffer.addFromWithRamp(outputIndex, 0, buffer.getReadPointer(inputIndex), numSamples, mi->prevGain, newGain);
			mi->prevGain = newGain;
		}
	}


	for (int outputIndex = 0; outputIndex < numAudioOutputs->intValue(); outputIndex++)
	{
		VolumeControl * outMI = &outputLines[outputIndex]->out;
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
	return data;
}

void MixerNode::loadJSONDataItemInternal(var data)
{
	Node::loadJSONDataItemInternal(data);
	autoSetNumAudioInputs();
	autoSetNumAudioOutputs();
	if (data.hasProperty("items")) itemsCC.loadJSONData(data.getProperty("items", var()));
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

OutputLineCC::OutputLineCC(int index) :
	ControllableContainer("Output " + String(index + 1)),
	index(index),
	out("Out", true)
{
	saveAndLoadRecursiveData = true;

	exclusiveMode = addBoolParameter("Exclusive Mode", "If checked, only one input will be active at a time", false);
	addChildControllableContainer(&out);
}

OutputLineCC::~OutputLineCC()
{
}

void OutputLineCC::setInputNumber(int inputNumber)
{
	while (mixerItems.size() > inputNumber)
	{
		MixerItem* mi = mixerItems[mixerItems.size() - 1];
		removeChildControllableContainer(mi);
		mixerItems.removeAllInstancesOf(mi);
	}

	while (mixerItems.size() < inputNumber)
	{
		MixerItem* mi = new MixerItem(mixerItems.size(), index);
		addChildControllableContainer(mi, true);
		mixerItems.add(mi);
	}
}

void OutputLineCC::updateActives(MixerItem* item)
{
	if (!exclusiveMode->boolValue()) return;

	if (item == nullptr)
	{
		if (mixerItems.size() == 0) return;
		item = mixerItems[0];
	}

	if (!item->active->boolValue())
	{
		item->active->setValue(true);
		return;
	}

	for (auto& mi : mixerItems)
	{
		if (mi == item) continue;
		mi->active->setValue(false);
	}
}

void OutputLineCC::onContainerParameterChanged(Parameter* p)
{
	if (p == exclusiveMode) updateActives(nullptr);
	
}

void OutputLineCC::onControllableFeedbackUpdate(ControllableContainer* cc, Controllable* c)
{
	if (exclusiveMode->boolValue())
	{
		if (MixerItem* m = dynamic_cast<MixerItem *>(cc))
		{
			if (c == m->active && m->active->boolValue()) updateActives(m);
		}
	}
}
