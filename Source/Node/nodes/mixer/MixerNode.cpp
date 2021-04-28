/*
  ==============================================================================

	MixerNode.cpp
	Created: 15 Nov 2020 8:42:42am
	Author:  bkupe

  ==============================================================================
*/

MixerNode::MixerNode(var params) :
	Node(getTypeString(), params, true, true, true, true)
{
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

	reorderContainers();
}

void MixerNode::updateAudioOutputsInternal()
{
	for (auto& il : inputLines) il->setNumOutputs(getNumAudioOutputs());

	while (mainOuts.size() > getNumAudioOutputs())
	{
		VolumeControl* vc = mainOuts[mainOuts.size() - 1];
		removeChildControllableContainer(vc);
		mainOuts.removeAllInstancesOf(vc);
	}

	while (mainOuts.size() < getNumAudioOutputs())
	{
		VolumeControl* o = new VolumeControl("Out" + String(mainOuts.size() + 1));
		addChildControllableContainer(o, true);
		mainOuts.add(o);
	}

	reorderContainers();
}

MixerItem* MixerNode::getMixerItem(int inputIndex, int outputIndex)
{
	return  inputLines[inputIndex]->mixerItems[outputIndex];
}

void MixerNode::reorderContainers()
{
	for (auto& il : inputLines) controllableContainers.removeAllInstancesOf(il);
	for (auto& o : mainOuts) controllableContainers.removeAllInstancesOf(o);

	for (auto& il : inputLines) controllableContainers.add(il);
	for (auto& o : mainOuts) controllableContainers.add(o);

	controllableContainerListeners.call(&ControllableContainerListener::controllableContainerReordered, this);
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

var MixerNode::getJSONData()
{
	var data = Node::getJSONData();
	var inData;
	var outData;
	for (auto& il : inputLines) inData.append(il->getJSONData());
	for (auto& o : mainOuts) outData.append(o->getJSONData());
	data.getDynamicObject()->setProperty("items", inData);
	data.getDynamicObject()->setProperty("mainOuts", outData);
	return data;
}

void MixerNode::loadJSONDataItemInternal(var data)
{
	Node::loadJSONDataItemInternal(data);
	autoSetNumAudioOutputs();
	autoSetNumAudioInputs();

	var inData = data.getProperty("items", var());
	var outData = data.getProperty("mainOuts", var());
	for (int i = 0; i < inData.size() && i < inputLines.size(); i++) inputLines[i]->loadJSONData(inData[i]);
	for (int i = 0; i < outData.size() && i < mainOuts.size(); i++) mainOuts[i]->loadJSONData(outData[i]);

	saveAndLoadRecursiveData = false; //tmp fix force here because previous versions had "containers" property which would force recursiveSave to true in CC::loadJSONData
}

void MixerNode::afterLoadJSONDataInternal()
{
	
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
	exclusiveIndex->forceSaveValue = true;
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
					if(!isCurrentlyLoadingData) exclusiveIndex->setValue(mixerItems.indexOf(mi)+1);
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
	if (item == nullptr) return;

	item->active->setValue(true); 
	
	for (auto& mi : mixerItems)
	{
		if(mi == item) continue;
		mi->active->setValue(false);
	}
}

void InputLineCC::setNumOutputs(int numOutputs)
{
	while (mixerItems.size() > numOutputs)
	{
		MixerItem* mi = mixerItems[mixerItems.size() - 1];
		removeChildControllableContainer(mi);
		mixerItems.removeAllInstancesOf(mi);
	}

	while (mixerItems.size() < numOutputs)
	{
		MixerItem* mi = new MixerItem(index, mixerItems.size());
		addChildControllableContainer(mi, true);
		mixerItems.add(mi);
	}

	exclusiveIndex->setRange(1, jmax(mixerItems.size(), 1));
}
