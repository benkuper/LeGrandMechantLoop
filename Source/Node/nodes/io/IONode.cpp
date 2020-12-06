/*
  ==============================================================================

	IONode.cpp
	Created: 15 Nov 2020 8:42:54am
	Author:  bkupe

  ==============================================================================
*/

#include "IONode.h"
#include "ui/IONodeViewUI.h"

IONode::IONode(var params, bool isInput) :
	Node(getTypeString(), params, !isInput, isInput),
	channelsCC("Channels"),
	isInput(isInput)
{
	viewUISize->setPoint(150, 180);

	channelsCC.saveAndLoadRecursiveData = true;
	addChildControllableContainer(&channelsCC);
}

void IONode::updateAudioInputsInternal()
{
	if (!isInput) updateIO();
}

void IONode::updateAudioOutputsInternal()
{
	if (isInput) updateIO();
}

void IONode::updateIO()
{
	int numChannels = isInput ? getNumAudioOutputs() : getNumAudioInputs();
	StringArray names = isInput ? audioOutputNames : audioInputNames;

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

void IONode::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
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

void IONode::processBlockBypassed(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
	buffer.clear();
}

var IONode::getJSONData()
{
	var data = Node::getJSONData();
	var gainData;
	for (auto& cc : channelsCC.controllableContainers) gainData.append(((IOChannel*)cc.get())->gain->floatValue());
	data.getDynamicObject()->setProperty("gains", gainData);
	return data;
}

void IONode::loadJSONDataInternal(var data)
{
	Node::loadJSONDataInternal(data);
	gainGhostData = data.getProperty("gains", var());
}

BaseNodeViewUI* IONode::createViewUI()
{
	return new IONodeViewUI(this);
}

void AudioInputNode::updatePlayConfigInternal()
{
	processor->setPlayConfigDetails(getNumAudioOutputs(), getNumAudioOutputs(), graph->getSampleRate(), graph->getBlockSize());
}

void AudioOutputNode::updatePlayConfigInternal()
{
	processor->setPlayConfigDetails(getNumAudioInputs(), getNumAudioInputs(), graph->getSampleRate(), graph->getBlockSize());
}

IOChannel::IOChannel(StringRef name) :
	ControllableContainer(name)
{
	gain = addFloatParameter("Gain", "Gain for this channel", 1, 0, 3);
	rms = addFloatParameter("RMS", "RMS for this channel", 0, 0, 1);
	rms->setControllableFeedbackOnly(true);
	active = addBoolParameter("Active", "Fast way to mute a channel", true);
}
