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
	Node(getTypeString(), params, true, true, true, true)
{
	saveAndLoadRecursiveData = true;

	showOutputGains = viewCC.addBoolParameter("Show Outputs Gain", "Show Output Gain", true);
	showOutputRMS = viewCC.addBoolParameter("Show Outputs RMS", "Show Output RMS", true);
	showOutputActives = viewCC.addBoolParameter("Show Outputs Active", "Show Output Active", true);
	showOutputExclusives = viewCC.addBoolParameter("Show Outputs Exclusives", "Show Output Exclusives", true);
	showItemGains = viewCC.addBoolParameter("Show Items Gain", "Show Items Gain", true);
	showItemActives = viewCC.addBoolParameter("Show Items Active", "Show Items Active", true);

	viewUISize->setPoint(170, 250);
}

void MixerNode::updateAudioInputsInternal()
{
	while (inputLines.size() > getNumAudioInputs())
	{
		InputLineCC* cc = inputLines[inputLines.size() - 1];
		inputLines.removeAllInstancesOf(cc);
		removeChildControllableContainer(cc);
	}

	while (inputLines.size() < getNumAudioInputs())
	{
		InputLineCC* cc = new InputLineCC(inputLines.size());
		cc->setNumOutputs(getNumAudioOutputs());
		addChildControllableContainer(cc, true);
		inputLines.add(cc);
	}
}

void MixerNode::updateAudioOutputsInternal()
{
	for (auto& il : inputLines) il->setNumOutputs(getNumAudioOutputs());

	while (mainOuts.size() > getNumAudioInputs())
	{
		VolumeControl* vc = mainOuts[mainOuts.size() - 1];
		removeChildControllableContainer(vc);
		mainOuts.removeAllInstancesOf(vc);
	}

	while (mainOuts.size() < getNumAudioInputs())
	{
		VolumeControl* o = new VolumeControl("Out" + String(mainOuts.size() + 1));
		addChildControllableContainer(o, true);
		mainOuts.add(o);
	}
}

MixerItem* MixerNode::getMixerItem(int inputIndex, int outputIndex)
{
	return  inputLines[inputIndex]->mixerItems[outputIndex];
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
		VolumeControl * outMI = mainOuts[outputIndex];
		float newGain = outMI->getGain();
		buffer.clear(outputIndex, 0, numSamples);
		buffer.addFromWithRamp(outputIndex, 0, tmpBuffer.getReadPointer(outputIndex), numSamples, outMI->prevGain, newGain);
		outMI->prevGain = newGain;
		
		outMI->updateRMS(buffer, outputIndex);
	}
}

void MixerNode::afterLoadJSONDataInternal()
{
	Node::afterLoadJSONDataInternal();
	autoSetNumAudioInputs();
	autoSetNumAudioOutputs();
}

BaseNodeViewUI* MixerNode::createViewUI()
{
	return new MixerNodeViewUI(this);
}

MixerItem::MixerItem(int inputIndex, int outputIndex, bool hasRMS) :
	VolumeControl("Out " + String(outputIndex + 1), hasRMS),
	inputIndex(inputIndex),
	outputIndex(outputIndex)
{
}

MixerItem::~MixerItem()
{
}

InputLineCC::InputLineCC(int index) :
	ControllableContainer("In " + String(index + 1)),
	index(index)
{
	saveAndLoadRecursiveData = true;
	exclusiveIndex = addIntParameter("Index", "Exclusive Index", 1, 1, 1, false);
	exclusiveIndex->canBeDisabledByUser = true;
}

InputLineCC::~InputLineCC()
{
}

void InputLineCC::onContainerParameterChanged(Parameter* p)
{
	if (p == exclusiveIndex) updateExclusiveOutput();
}

void InputLineCC::onControllableStateChanged(Controllable* c)
{
	if (c == exclusiveIndex) updateExclusiveOutput();
}

void InputLineCC::onControllableFeedbackUpdate(ControllableContainer* cc, Controllable* c)
{
	if (MixerItem* mi = dynamic_cast<MixerItem*>(cc))
	{
		if (c == mi->active)
		{
			if (mi->active->boolValue())
			{
				if (exclusiveIndex->enabled && mixerItems.indexOf(mi) != exclusiveIndex->intValue() - 1)
				{
					exclusiveIndex->setValue(mixerItems.indexOf(mi)+1);
				}
			}
		}
	}
}

void InputLineCC::updateExclusiveOutput()
{
	if (!exclusiveIndex->enabled) return;

	int index = jlimit(0, mixerItems.size(), exclusiveIndex->intValue() - 1);
	
	MixerItem* item = mixerItems[index];
	
	item->active->setValue(true); 
	
	for (auto& mi : mixerItems)
	{
		if(mi == item) continue;
		mi->active->setValue(false);
	}
}

void InputLineCC::setNumOutputs(int inputNumber)
{
	while (mixerItems.size() > inputNumber)
	{
		MixerItem* mi = mixerItems[mixerItems.size() - 1];
		removeChildControllableContainer(mi);
		mixerItems.removeAllInstancesOf(mi);
	}

	while (mixerItems.size() < inputNumber)
	{
		MixerItem* mi = new MixerItem(index, mixerItems.size());
		addChildControllableContainer(mi, true);
		mixerItems.add(mi);
	}

	exclusiveIndex->setRange(1, jmax(mixerItems.size(), 1));
}