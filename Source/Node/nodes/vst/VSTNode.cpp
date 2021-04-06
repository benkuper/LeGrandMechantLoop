/*
  ==============================================================================

	VSTNode.cpp
	Created: 15 Nov 2020 8:42:29am
	Author:  bkupe

  ==============================================================================
*/

#include "VSTNode.h"
#include "ui/VSTNodeViewUI.h"
#include "Transport/Transport.h"
#include "VSTLinkedParameter.h"

VSTNode::VSTNode(var params) :
	Node(getTypeString(), params),
	currentDevice(nullptr),
	isSettingVST(false),
	currentPreset(nullptr),
	vstNotifier(5)
{
	pluginParam = new VSTPluginParameter("VST", "The VST to use");
	ControllableContainer::addParameter(pluginParam);

	midiParam = new MIDIDeviceParameter("MIDI Device", true, false);
	ControllableContainer::addParameter(midiParam);

	presetEnum = addEnumParameter("Preset", "Load a preset");


	setAudioInputs(2);
	setAudioOutputs(2);

	viewUISize->setPoint(200, 150);
}

VSTNode::~VSTNode()
{

}

void VSTNode::clearItem()
{
	Node::clearItem();
	setMIDIDevice(nullptr);
	setupVST(nullptr);
}

void VSTNode::setMIDIDevice(MIDIInputDevice* d)
{
	if (currentDevice == d) return;
	if (currentDevice != nullptr)
	{
		currentDevice->removeMIDIInputListener(this);
	}

	currentDevice = d;

	if (currentDevice != nullptr)
	{
		currentDevice->addMIDIInputListener(this);
	}
}

void VSTNode::setupVST(PluginDescription* description)
{
	ScopedSuspender sp(processor);

	isSettingVST = true;


	if (vst != nullptr)
	{
		vst->releaseResources();
	}

	if (vstParamsCC != nullptr)
	{
		removeChildControllableContainer(vstParamsCC.get());
		vstParamsCC.reset();
	}

	vstNotifier.addMessage(new VSTEvent(VSTEvent::VST_REMOVED, this));
	vstNotifier.triggerAsyncUpdate();

	presets.clear();

	if (description == nullptr)
	{
		vst.reset();
	}
	else
	{
		try
		{
			String errorMessage;
			vst = VSTManager::getInstance()->formatManager->createPluginInstance(*description, processor->getSampleRate(), processor->getBlockSize(), errorMessage);

			if (errorMessage.isNotEmpty())
			{
				NLOGERROR(niceName, "VST Load error : " << errorMessage);
			}
		}
		catch (std::exception e)
		{
			NLOGERROR(niceName, "Error while loading plugin : " << e.what());
		}

		if (vst != nullptr)
		{
			vst->setPlayHead(Transport::getInstance());
			setAudioInputs(description->numInputChannels);
			setAudioOutputs(description->numOutputChannels);
			setMIDIIO(vst->acceptsMidi(), false);

			vstParamsCC.reset(new VSTParameterContainer(vst.get()));
			addChildControllableContainer(vstParamsCC.get());
		}
		else
		{
			setMIDIIO(false, false);
		}
	}

	isSettingVST = false;
	updatePlayConfig();
	updatePresetEnum();

	vstNotifier.addMessage(new VSTEvent(VSTEvent::VST_SET, this));
}

String VSTNode::getVSTState()
{
	if (vst == nullptr) return "";

	MemoryBlock b;
	vst->getStateInformation(b);
	return b.toBase64Encoding();
}

void VSTNode::setVSTState(const String& data)
{
	if (vst == nullptr) return;
	if (data.isEmpty()) return;

	MemoryBlock b;
	if (b.fromBase64Encoding(data)) vst->setStateInformation(b.getData(), b.getSize());
}

void VSTNode::updatePresetEnum(const String& setPresetName)
{
	String nameToChoose = setPresetName;
	if (nameToChoose.isEmpty())
	{
		if ((int)presetEnum->getValueData() < 1000) nameToChoose = presetEnum->getValueKey();
		else nameToChoose = "None";
	}

	presetEnum->clearOptions();
	presetEnum->addOption("None", 0, false);
	for (int i = 0; i < presets.size(); i++) presetEnum->addOption(presets[i]->name, i + 1);
	presetEnum->addOption("Save Current", 1000);
	presetEnum->addOption("Add New", 1001);
	presetEnum->addOption("Delete current", 1002);

	presetEnum->setValueWithKey(nameToChoose);
}

void VSTNode::updatePlayConfigInternal()
{
	Node::updatePlayConfigInternal();

	if (vst != nullptr && !isSettingVST)
	{
		vst->setRateAndBufferSizeDetails(processor->getSampleRate(), processor->getBlockSize());
		vst->prepareToPlay(processor->getSampleRate(), processor->getBlockSize());
	}
}

void VSTNode::onContainerParameterChangedInternal(Parameter* p)
{
	Node::onContainerParameterChangedInternal(p);
	if (p == pluginParam) setupVST(pluginParam->getPluginDescription());
	if (p == midiParam) setMIDIDevice(midiParam->inputDevice);
	if (p == presetEnum)
	{
		int d = presetEnum->getValueData();
		if (d == 1000)
		{
			if (currentPreset != nullptr)
			{
				currentPreset->data = getVSTState();
			}

			presetEnum->setValueWithKey(currentPreset != nullptr ? currentPreset->name : "None");
		}
		else if (d == 1001)
		{
			AlertWindow nameWindow("Add a preset", "Set the name for the new preset", AlertWindow::AlertIconType::NoIcon);

			nameWindow.addTextEditor("name", "New preset", "Name");
			nameWindow.addButton("OK", 1, KeyPress(KeyPress::returnKey));
			nameWindow.addButton("Cancel", 0, KeyPress(KeyPress::escapeKey));

			int result = nameWindow.runModalLoop();
			if (result)
			{
				String pName = nameWindow.getTextEditorContents("name");
				presets.add(new VSTPreset({ pName, getVSTState() }));
				updatePresetEnum(pName);
			}
		}
		else if (d == 1002)
		{
			presets.removeObject(currentPreset);
			currentPreset = nullptr;
			updatePresetEnum();
		}
		else if (d == 0)
		{
			currentPreset = nullptr;
		}
		else
		{
			currentPreset = presets[(int)presetEnum->getValueData() - 1];
			setVSTState(currentPreset->data);
		}
	}
}


void VSTNode::midiMessageReceived(const MidiMessage& m)
{
	midiCollector.addMessageToQueue(m); //ugly hack to have at least events sorted, but sampleNumber should be exact
}

void VSTNode::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock)
{
	if (vst != nullptr) vst->prepareToPlay(sampleRate, maximumExpectedSamplesPerBlock);
	if (sampleRate != 0) midiCollector.reset(sampleRate);
}

void VSTNode::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
	if (vst != nullptr)
	{
		if (currentDevice != nullptr)
		{
			inMidiBuffer.clear();
			midiCollector.removeNextBlockOfMessages(inMidiBuffer, buffer.getNumSamples());
		}

		vst->processBlock(buffer, inMidiBuffer);
	}

	inMidiBuffer.clear();
}

void VSTNode::processBlockBypassed(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
	if (getNumAudioInputs() == 0) buffer.clear();
	Node::processBlockBypassed(buffer, midiMessages);
}

var VSTNode::getJSONData()
{
	var data = Node::getJSONData();
	if (vst != nullptr) data.getDynamicObject()->setProperty("vstState", getVSTState());

	if (vstParamsCC != nullptr) data.getDynamicObject()->setProperty("vstParams", vstParamsCC->getJSONData());

	if (presets.size() > 0)
	{
		var presetData = new DynamicObject();
		for (auto& p : presets) presetData.getDynamicObject()->setProperty(p->name, p->data);

		data.getDynamicObject()->setProperty("presets", presetData);
	}


	return data;
}

void VSTNode::loadJSONDataItemInternal(var data)
{
	Node::loadJSONDataItemInternal(data);

	setVSTState(data.getProperty("vstState", ""));

	if (vstParamsCC != nullptr) vstParamsCC->loadJSONData(data.getProperty("vstParams", var()));

	presets.clear();
	var presetData = data.getProperty("presets", var());
	if (presetData.isObject())
	{
		NamedValueSet pData = presetData.getDynamicObject()->getProperties();
		for (auto& pd : pData) presets.add(new VSTPreset({ pd.name.toString(), pd.value }));
	}
	updatePresetEnum();
}

BaseNodeViewUI* VSTNode::createViewUI()
{
	return new VSTNodeViewUI(this);
}
