/*
  ==============================================================================

	IONode.cpp
	Created: 15 Nov 2020 8:42:54am
	Author:  bkupe

  ==============================================================================
*/

IONode::IONode(StringRef name, var params, bool isInput) :
	Node(name, params, !isInput, isInput),
    isInput(isInput),
	channelsCC("Channels"),
	isRoot(false),
	realNumInputs(0),
	realNumOutputs(0),
	ghostNumChannels(-1)
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
			numAudioOutputs = addIntParameter("Num Channels", "Number of channels to get from the sound card", 2, 0, 32);
			numAudioOutputs->isSavable = false; //handled manually
			if (ghostNumChannels >= 0) numAudioOutputs->setValue(ghostNumChannels);
			autoSetNumAudioOutputs();
		}
		else
		{
			numAudioInputs = addIntParameter("Num Channels", "Number of channels to get to the sound card", 2, 0, 32);
			numAudioInputs->isSavable = false; //handled manually
			if (ghostNumChannels >= 0) numAudioInputs->setValue(ghostNumChannels);
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
	realNumInputs = inputNames.size();

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

	Node::setAudioInputs(actualInputNames, updateConfig);
}

void IONode::setAudioOutputs(const StringArray& outputNames, bool updateConfig)
{
	realNumOutputs = outputNames.size();

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

	Node::setAudioOutputs(actualOutputNames, updateConfig);
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
	//StringArray names = isInput ? audioOutputNames : audioInputNames;

	//remove surplus
	while (channelsCC.controllableContainers.size() > numChannels)
	{
		channelsCC.removeChildControllableContainer(channelsCC.controllableContainers[channelsCC.controllableContainers.size() - 1]);
	}

	//rename existing
	/*
	for (int i = 0; i < channelsCC.controllableContainers.size(); i++)
	{
		String s = names[i];
		channelsCC.controllableContainers[i]->setNiceName(s);
	}
	*/

	//add more
	while (channelsCC.controllableContainers.size() < numChannels)
	{
		int index = channelsCC.controllableContainers.size();
		String s = String(index + 1);// names[index];

		VolumeControl * channel = new VolumeControl(s, true);
		channelsCC.addChildControllableContainer(channel, true);
		if (index < channelsGhostData.size()) channel->loadJSONData(channelsGhostData[index]);
	}
}

void IONode::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
	for (int i = 0; i < channelsCC.controllableContainers.size() && i < buffer.getNumChannels(); i++)
	{
		VolumeControl* chan = (VolumeControl *)channelsCC.controllableContainers[i].get();

		if (chan->rms == nullptr || chan->gain == nullptr) return;

		chan->applyGain(i, buffer);
	}
}

void IONode::processBlockBypassed(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
	buffer.clear();
}

var IONode::getJSONData()
{
	var data = Node::getJSONData();
	var channelsData;
	for (auto& cc : channelsCC.controllableContainers) channelsData.append(((VolumeControl*)cc.get())->getJSONData());
	data.getDynamicObject()->setProperty("channels", channelsData);
	if (isRoot) data.getDynamicObject()->setProperty("numChannels", isInput ? numAudioOutputs->intValue() : numAudioInputs->intValue());
	return data;
}

void IONode::loadJSONDataItemInternal(var data)
{
	Node::loadJSONDataItemInternal(data);
	channelsGhostData = data.getProperty("channels", var());
	ghostNumChannels = data.getProperty("numChannels", -1);
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