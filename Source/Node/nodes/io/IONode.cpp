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
	GenericNodeProcessor(n, !isInput, isInput),
	channelsCC("Channels"),
	isInput(isInput)
{
	nodeRef->viewUISize->setPoint(150, 180);

	channelsCC.saveAndLoadRecursiveData = true;
	addChildControllableContainer(&channelsCC);
}

void IOProcessor::updateInputsFromNodeInternal()
{
	if (!isInput) updateIOFromNode();
}

void IOProcessor::updateOutputsFromNodeInternal()
{
	if (isInput) updateIOFromNode();
}

void IOProcessor::updateIOFromNode()
{
	int numChannels = isInput ? nodeRef->numOutputs : nodeRef->numInputs;
	StringArray names = isInput ? nodeRef->audioOutputNames : nodeRef->audioInputNames;

	//remove surplus
	while (channelsCC.controllableContainers.size() > numChannels)
	{
		channelsCC.removeChildControllableContainer(channelsCC.controllableContainers[channelsCC.controllableContainers.size() - 1]);
	}

	//rename existing
	for (int i = 0; i < channelsCC.controllableContainers.size(); i++)
	{
		String s = names[i];
		channelsCC.controllableContainers[i]->setNiceName(s);
	}

	//add more
	while (channelsCC.controllableContainers.size() < numChannels)
	{
		int index = channelsCC.controllableContainers.size();
		String s = names[index];

		IOChannel* channel = new IOChannel(s);
		channelsCC.addChildControllableContainer(channel, true);
		if (index < gainGhostData.size()) channel->gain->setValue(gainGhostData[index]);

	}

}

void IOProcessor::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
	for (int i = 0; i < channelsCC.controllableContainers.size() && i < buffer.getNumChannels(); i++)
	{
		IOChannel* chan = (IOChannel *)channelsCC.controllableContainers[i].get();

		if (chan->rms == nullptr || chan->gain == nullptr) return;

		float gain = chan->active->boolValue() ? chan->gain->floatValue() : 0;
		buffer.applyGain(i, 0, buffer.getNumSamples(), gain);

		float rms = buffer.getRMSLevel(i, 0, buffer.getNumSamples());
		float curVal = chan->rms->floatValue();
		float targetVal = chan->rms->getLerpValueTo(rms, rms > curVal ? .8f : .2f);
		chan->rms->setValue(targetVal);
	}
}

void IOProcessor::processBlockBypassed(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
	buffer.clear();
}

var IOProcessor::getJSONData()
{
	var data = GenericNodeProcessor::getJSONData();
	var gainData;
	for (auto& cc : channelsCC.controllableContainers) gainData.append(((IOChannel*)cc.get())->gain->floatValue());
	data.getDynamicObject()->setProperty("gains", gainData);
	return data;
}

void IOProcessor::loadJSONDataInternal(var data)
{
	GenericNodeProcessor::loadJSONDataInternal(data);
	gainGhostData = data.getProperty("gains", var());
}

NodeViewUI* IOProcessor::createNodeViewUI()
{
	return new IONodeViewUI((GenericNode<IOProcessor>*)nodeRef.get());
}

void AudioInputProcessor::updatePlayConfig()
{
	setPlayConfigDetails(nodeRef->numOutputs, nodeRef->numOutputs, getSampleRate(), getBlockSize());
}

void AudioOutputProcessor::updatePlayConfig()
{
	setPlayConfigDetails(nodeRef->numInputs, nodeRef->numInputs, getSampleRate(), getBlockSize());
}

IOChannel::IOChannel(StringRef name) :
	ControllableContainer(name)
{
	gain = addFloatParameter("Gain", "Gain for this channel", 1, 0, 3);
	rms = addFloatParameter("RMS", "RMS for this channel", 0, 0, 1);
	rms->setControllableFeedbackOnly(true);
	active = addBoolParameter("Active", "Fast way to mute a channel", true);
}
