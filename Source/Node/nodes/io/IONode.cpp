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
	GenericNodeProcessor(n, !isInput, isInput, false),
	rmsCC("RMS"),
	gainCC("Gain"),
	isInput(isInput)
{
	nodeRef->viewUISize->setPoint(120, 150);

	addChildControllableContainer(&rmsCC);
	addChildControllableContainer(&gainCC);

	rmsCC.editorIsCollapsed = true;

	updateIOFromNode();
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
		int index = gainCC.controllables.size();
		String s = names[index];

		FloatParameter* p = rmsCC.addFloatParameter(s, "RMS for this input", 0, 0, 1);
		p->setControllableFeedbackOnly(true);

		FloatParameter * gp = gainCC.addFloatParameter(s, "Gain for this input", 1, 0, 2);
		if (index < gainGhostData.size()) gp->setValue(gainGhostData[index]);

	}

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

void IOProcessor::processBlockBypassed(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
	buffer.clear();
}

var IOProcessor::getJSONData()
{
	var data = GenericNodeProcessor::getJSONData();
	var gainData;
	for (auto& c : gainCC.controllables) gainData.append(((FloatParameter*)c)->floatValue());
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
