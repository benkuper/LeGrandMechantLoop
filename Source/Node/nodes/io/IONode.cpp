/*
  ==============================================================================

	IONode.cpp
	Created: 15 Nov 2020 8:42:54am
	Author:  bkupe

  ==============================================================================
*/

#include "IONode.h"
#include "ui/IONodeViewUI.h"

IONode::IONode(StringRef name, var params, bool isInput) :
	Node(name, params, !isInput, isInput),
    isInput(isInput),
	channelsCC("Channels"),
	isRoot(false)
{
	viewUISize->setPoint(150, 180);

	channelsCC.saveAndLoadRecursiveData = true;
	addChildControllableContainer(&channelsCC);
}

void IONode::setIsRoot(bool value)
{
	if (isRoot == value) return;
	isRoot = value;
	if (isRoot)
	{
		if (isInput)
		{
			numAudioOutputs = addIntParameter("Channels", "Number of channels to get from the sound card", 2, 0, 32);
			autoSetNumAudioOutputs();
		}
		else
		{
			numAudioInputs = addIntParameter("Channels", "Number of channels to get to the sound card", 2, 0, 32);
			autoSetNumAudioInputs();
		}
	}
	else
	{
		if (isInput)
		{
			removeControllable(numAudioOutputs);
			numAudioOutputs = nullptr;
		}
		else
		{
			removeControllable(numAudioInputs);
			numAudioInputs = nullptr;
		}
	}
}


void IONode::setAudioInputs(const StringArray& inputNames, bool updateConfig)
{
	if (!isRoot)
	{
		Node::setAudioInputs(inputNames, updateConfig);
		return;
	}

	StringArray actualInputNames;
	for (int i = 0; i < numAudioInputs->intValue(); i++)
	{
		actualInputNames.add(i < inputNames.size() ? inputNames[i] : "Output " + String(i + 1));
	}

	Node::setAudioOutputs(actualInputNames);
}

void IONode::setAudioOutputs(const StringArray& outputNames, bool updateConfig)
{
	if (!isRoot)
	{
		Node::setAudioOutputs(outputNames, updateConfig);
		return;
	}

	StringArray actualOutputNames;
	for (int i = 0; i < numAudioOutputs->intValue(); i++)
	{
		actualOutputNames.add(i < outputNames.size() ? outputNames[i] : "Input " + String(i + 1));
	}

	Node::setAudioInputs(actualOutputNames);
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

		VolumeControl * channel = new VolumeControl(s);
		channelsCC.addChildControllableContainer(channel, true);
		if (index < gainGhostData.size()) channel->gain->setValue(gainGhostData[index]);
	}
}

void IONode::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
	for (int i = 0; i < channelsCC.controllableContainers.size() && i < buffer.getNumChannels(); i++)
	{
		VolumeControl* chan = (VolumeControl *)channelsCC.controllableContainers[i].get();

		if (chan->rms == nullptr || chan->gain == nullptr) return;

		float gain = chan->getGain();
		buffer.applyGainRamp(i, 0, buffer.getNumSamples(), chan->prevGain, gain);
		chan->prevGain = gain;

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
	for (auto& cc : channelsCC.controllableContainers) gainData.append(((VolumeControl*)cc.get())->gain->floatValue());
	data.getDynamicObject()->setProperty("gains", gainData);
	return data;
}

void IONode::loadJSONDataItemInternal(var data)
{
	Node::loadJSONDataItemInternal(data);
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