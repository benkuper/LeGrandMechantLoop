/*
  ==============================================================================

	PatchBayNode.cpp
	Created: 26 Nov 2024 12:04:38pm
	Author:  bkupe

  ==============================================================================
*/

#include "Node/NodeIncludes.h"
#include "PatchBayNode.h"

PatchBayNode::PatchBayNode(var params) :
	Node(getTypeString(), params, true, true, true, false, false, false),
	presetsCC("Presets"),
	connections("Patches")
{
	presetEnum = presetsCC.addEnumParameter("Presets", "Patch configurations");
	reloadPreset = presetsCC.addTrigger("Reload Preset", "Reload the current preset");
	addPreset = presetsCC.addTrigger("Add Preset", "Add a new preset");
	savePreset = presetsCC.addTrigger("Save Preset", "Save the current state as a preset");
	newPresetName = presetsCC.addStringParameter("New Preset Name", "Name for the new preset when triggering add preset", "New Preset");
	updatePresetName = presetsCC.addTrigger("Update Preset Name", "Update the name of the current preset");
	deletePreset = presetsCC.addTrigger("Delete Preset", "Delete the current preset");
	addChildControllableContainer(&presetsCC);
	addChildControllableContainer(&connections);

	presets = var(new DynamicObject());

	connections.addBaseManagerListener(this);
	connections.selectItemWhenCreated = false;
}

void PatchBayNode::updatePresetEnum()
{
	Array<EnumParameter::EnumValue*> options;
	options.add(new EnumParameter::EnumValue("None", var()));
	NamedValueSet& nvSet = presets.getDynamicObject()->getProperties();
	for (auto& nv : nvSet) options.add(new EnumParameter::EnumValue(nv.name.toString(), nv.name.toString()));
	presetEnum->setOptions(options);
}

void PatchBayNode::loadCurrentPreset()
{
	ScopedSuspender sp(processor);

	if (presetEnum->getValue().isVoid())
	{
		connections.clear();
	}
	else
	{
		var data = presets.getProperty(presetEnum->getValue().toString(), var());
		if (data.isVoid())
		{
			NLOGWARNING(niceName, "No data in preset " << presetEnum->getValue().toString());
		}
		else
		{
			connections.loadJSONData(data);
		}
	}
}

void PatchBayNode::itemAdded(PatchConnection* item)
{
	item->updateInputOptions(audioInputNames);
	item->updateOutputOptions(audioOutputNames);
}

void PatchBayNode::itemsAdded(Array<PatchConnection*> items)
{
	for (auto& c : items) itemAdded(c);
}

void PatchBayNode::itemRemoved(PatchConnection* item)
{
	//
}

void PatchBayNode::onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c)
{
	Node::onControllableFeedbackUpdateInternal(cc, c);

	if (cc == &presetsCC)
	{
		if (c == presetEnum)
		{
			loadCurrentPreset();
		}
		else if (c == reloadPreset)
		{
			loadCurrentPreset();
		}
		else if (c == addPreset)
		{
			String s = newPresetName->stringValue();
			if (s.isEmpty()) s = "New preset";
			String uniqueName = s;
			int i = 1;
			while (presets.hasProperty(uniqueName))
			{
				uniqueName = s + " " + String(i);
				i++;
			}
			presets.getDynamicObject()->setProperty(uniqueName, connections.getJSONData());
			updatePresetEnum();
			presetEnum->setValueWithKey(uniqueName);
		}
		else if (c == savePreset)
		{
			if (presetEnum->getValueKey().isNotEmpty())
			{
				presets.getDynamicObject()->setProperty(presetEnum->getValueKey(), connections.getJSONData());
				presetEnum->setValueWithKey(presetEnum->getValueKey());
			}
		}
		else if (c == updatePresetName)
		{
			presets.getDynamicObject()->setProperty(newPresetName->stringValue(), presets.getProperty(presetEnum->getValueKey(),var()));
			presets.getDynamicObject()->removeProperty(presetEnum->getValueKey());
			updatePresetEnum();
			presetEnum->setValueWithKey(newPresetName->stringValue());
		}
		else if (c == deletePreset)
		{
			//delete preset
			presets.getDynamicObject()->removeProperty(presetEnum->getValueKey());
			updatePresetEnum();
		}
	}

	else if (c->parentContainer == customInputNamesCC.get())
	{
		for (auto& c : connections.items) c->updateInputOptions(audioInputNames);
	}

	else if (c->parentContainer == customOutputNamesCC.get())
	{
		for (auto& c : connections.items) c->updateOutputOptions(audioOutputNames);
	}
}

void PatchBayNode::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
	AudioBuffer<float> tmpBuffer(buffer.getNumChannels(), buffer.getNumSamples());
	tmpBuffer.clear();

	for (auto& c : connections.items)
	{
		int channelIn = audioInputNames.indexOf(c->input->getValue().toString());
		int channelOut = audioOutputNames.indexOf(c->output->getValue().toString());

		if (channelIn == -1 || channelOut == -1) return;

		tmpBuffer.addFrom(channelOut, 0, buffer, channelIn, 0, buffer.getNumSamples());
	}

	buffer.makeCopyOf(tmpBuffer, true);
}

var PatchBayNode::getJSONData()
{
	var data = Node::getJSONData();
	data.getDynamicObject()->setProperty("presets", presets);
	data.getDynamicObject()->setProperty("connections", connections.getJSONData());
	return data;
}

void PatchBayNode::loadJSONDataItemInternal(var data)
{
	presets = data.getProperty("presets", var(new DynamicObject()));
	updatePresetEnum();
	Node::loadJSONDataItemInternal(data);
	connectionsData = data.getProperty("connections", var());
}

void PatchBayNode::afterLoadJSONDataInternal()
{
	Node::afterLoadJSONDataInternal();
	connections.loadJSONData(connectionsData);
}

PatchConnection::PatchConnection(var params)
{
	input = addEnumParameter("Input", "Input");
	output = addEnumParameter("Output", "Output");
}

void PatchConnection::updateInputOptions(StringArray options, const String& forceNewVal)
{
	String tmpInput;
	if (input->getValueKey().isNotEmpty()) tmpInput = input->getValueKey();
	Array<EnumParameter::EnumValue*> values;

	for (int i = 0; i < options.size(); i++) values.add(new EnumParameter::EnumValue(options[i], options[i]));

	input->setOptions(values);
	if (forceNewVal.isNotEmpty()) input->setValueWithKey(forceNewVal);
	else if (tmpInput.isNotEmpty()) input->setValueWithKey(tmpInput);
	else if (ghostInput.isNotEmpty()) input->setValueWithKey(ghostInput);

	ghostInput = tmpInput;
}

void PatchConnection::updateOutputOptions(StringArray options, const String& forceNewVal)
{
	String tmpOutput;
	if (output->getValueKey().isNotEmpty()) tmpOutput = output->getValueKey();
	Array<EnumParameter::EnumValue*> values;

	for (int i = 0; i < options.size(); i++) values.add(new EnumParameter::EnumValue(options[i], options[i]));

	output->setOptions(values);
	if (forceNewVal.isNotEmpty()) output->setValueWithKey(forceNewVal);
	else if (tmpOutput.isNotEmpty()) output->setValueWithKey(tmpOutput);
	else if (ghostOutput.isNotEmpty()) output->setValueWithKey(ghostOutput);

	ghostOutput = tmpOutput;
}
