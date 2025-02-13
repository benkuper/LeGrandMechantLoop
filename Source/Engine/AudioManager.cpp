/*
  ==============================================================================

	AudioManager.cpp
	Created: 15 Nov 2020 8:44:58am
	Author:  bkupe

  ==============================================================================
*/

#include "ui/AudioManagerEditor.h"
#include "Transport/Transport.h"

juce_ImplementSingleton(AudioManager)

AudioManager::AudioManager() :
	ControllableContainer("Audio Settings"),
	graphIDIncrement(GRAPH_START_ID)
	//;isSettingUp(false)
{
	showWarningInUI = true;

	am.addAudioCallback(this);
	am.addChangeListener(this);
	am.initialiseWithDefaultDevices(0, 2);

	AudioDeviceManager::AudioDeviceSetup setup(am.getAudioDeviceSetup());
	setup.sampleRate = 48000;
	setup.bufferSize = 128;
	setup.useDefaultInputChannels = false;
	setup.useDefaultOutputChannels = true;

	//am.setAudioDeviceSetup(setup, false);

	am.addAudioCallback(&player);

	graph.reset();

	setup = am.getAudioDeviceSetup();
	currentSampleRate = setup.sampleRate;
	currentBufferSize = setup.bufferSize;

	for (auto& d : am.getAvailableDeviceTypes()) d->addListener(this);

	std::unique_ptr<AudioProcessorGraph::AudioGraphIOProcessor> procIn(new AudioProcessorGraph::AudioGraphIOProcessor(AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode));

	std::unique_ptr<AudioProcessorGraph::AudioGraphIOProcessor> procOut(new AudioProcessorGraph::AudioGraphIOProcessor(AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode));

	std::unique_ptr<AudioProcessorGraph::AudioGraphIOProcessor> midiIn(new AudioProcessorGraph::AudioGraphIOProcessor(AudioProcessorGraph::AudioGraphIOProcessor::midiInputNode));

	std::unique_ptr<AudioProcessorGraph::AudioGraphIOProcessor> midiOut(new AudioProcessorGraph::AudioGraphIOProcessor(AudioProcessorGraph::AudioGraphIOProcessor::midiOutputNode));



	graph.addNode(std::move(procIn), AudioProcessorGraph::NodeID(AUDIO_GRAPH_INPUT_ID));
	graph.addNode(std::move(procOut), AudioProcessorGraph::NodeID(AUDIO_GRAPH_OUTPUT_ID));
	graph.addNode(std::move(midiIn), AudioProcessorGraph::NodeID(MIDI_GRAPH_INPUT_ID));
	graph.addNode(std::move(midiOut), AudioProcessorGraph::NodeID(MIDI_GRAPH_OUTPUT_ID));

	player.setProcessor(&graph);

	numAudioInputs = setup.inputChannels.countNumberOfSetBits();
	numAudioOutputs = setup.outputChannels.countNumberOfSetBits();

	graph.setPlayConfigDetails(numAudioInputs, numAudioOutputs, currentSampleRate, currentBufferSize);
	graph.prepareToPlay(currentSampleRate, currentBufferSize);

	Engine::mainEngine->addEngineListener(this);
}

AudioManager::~AudioManager()
{
	Engine::mainEngine->removeEngineListener(this);

	am.removeAudioCallback(&player);
	graph.clear();
	player.setProcessor(nullptr);
	am.removeAudioCallback(this);
	am.removeChangeListener(this);
}

int AudioManager::getNewGraphID()
{
	return graphIDIncrement++;
}

void AudioManager::updateGraph()
{
	graph.suspendProcessing(true);

	AudioDeviceManager::AudioDeviceSetup setup = am.getAudioDeviceSetup();
	currentSampleRate = setup.sampleRate;
	currentBufferSize = setup.bufferSize;

	numAudioInputs = setup.inputChannels.countNumberOfSetBits();
	numAudioOutputs = setup.outputChannels.countNumberOfSetBits();

	graph.setPlayConfigDetails(numAudioInputs, numAudioOutputs, currentSampleRate, currentBufferSize);
	graph.prepareToPlay(currentSampleRate, currentBufferSize);

	audioManagerListeners.call(&AudioManagerListener::audioSetupChanged);

	graph.setPlayHead(Transport::getInstance());

	graph.suspendProcessing(false);
}


void AudioManager::audioDeviceIOCallbackWithContext
#if RPISAFEMODE
(const float** inputChannelData,
	int numInputChannels,
	float** outputChannelData,
	int numOutputChannels,
	int numSamples,
	const AudioIODeviceCallbackContext& context)
#else
//7.0.3
(const float* const* inputChannelData,
	int numInputChannels,
	float* const* outputChannelData,
	int numOutputChannels,
	int numSamples,
	const AudioIODeviceCallbackContext& context)
#endif

{
	for (int i = 0; i < numOutputChannels; ++i) FloatVectorOperations::clear(outputChannelData[i], numSamples);
	//for (int i = 0; i < jmin(numInputChannels, numOutputChannels); ++i) FloatVectorOperations::copy(outputChannelData[i], inputChannelData[i], numSamples);
}


void AudioManager::loadAudioConfig(juce::var fallbackConfig)
{
	if (lastUserState != nullptr)
	{
		String s = am.initialise(0, 2, lastUserState.get(), false);
		if (s.isNotEmpty())
		{
			setWarningMessage("Could not load audio settings: " + s, "device");
			if (Engine::mainEngine->isLoadingFile) LOGWARNING(getWarningMessage("device"));
		}
		else
		{
			clearWarning("device");
		}
	}

	if (am.getCurrentAudioDevice() == nullptr)
	{
		String deviceType = fallbackConfig.getProperty("deviceType", "").toString();
		if (deviceType.isNotEmpty())
		{
			am.initialise(0, 2, nullptr, false);
			am.setCurrentAudioDeviceType(deviceType, true);
		}
		else
		{
			am.initialise(0, 2, nullptr, false);
		}
	}

	AudioDeviceManager::AudioDeviceSetup setup = am.getAudioDeviceSetup();
	if (am.getCurrentAudioDevice() != nullptr && am.getCurrentAudioDevice()->getActiveOutputChannels() == 0)
	{
		if (fallbackConfig.isObject())
		{
			setup.inputDeviceName = fallbackConfig.getProperty("inputDeviceName", "").toString();
			setup.outputDeviceName = fallbackConfig.getProperty("outputDeviceName", "").toString();
			setup.sampleRate = fallbackConfig.getProperty("sampleRate", 48000).toString().getDoubleValue();
			setup.bufferSize = fallbackConfig.getProperty("bufferSize", 128).toString().getIntValue();

			BigInteger inputChannels;
			MemoryBlock bIn;
			bIn.fromBase64Encoding(fallbackConfig.getProperty("inputChannels", "").toString());
			inputChannels.loadFromMemoryBlock(bIn);

			BigInteger outputChannels;
			MemoryBlock bOut;
			bOut.fromBase64Encoding(fallbackConfig.getProperty("outputChannels", "").toString());
			outputChannels.loadFromMemoryBlock(bOut);

			setup.inputChannels = inputChannels;
			setup.outputChannels = outputChannels;

			am.setAudioDeviceSetup(setup, false);
		}
	}

}

void AudioManager::audioDeviceAboutToStart(AudioIODevice* device)
{
}

void AudioManager::audioDeviceStopped()
{
}

void AudioManager::audioDeviceListChanged()
{
	if (targetDeviceName.isNotEmpty() && (am.getCurrentAudioDevice() == nullptr || am.getCurrentAudioDevice()->getName() != targetDeviceName))
	{
		bool found = false;
		for (auto& d : am.getAvailableDeviceTypes())
		{
			for (auto& n : d->getDeviceNames())
			{
				if (n == targetDeviceName)
				{
					found = true;
					break;
				}
			}
		}

		if (found) loadAudioConfig();
	}
}

void AudioManager::changeListenerCallback(ChangeBroadcaster* source)
{
	std::unique_ptr<XmlElement> newState = am.createStateXml();
	if (lastUserState == nullptr || !lastUserState->isEquivalentTo(newState.get(), true))
	{
		//LOG("Audio setup changed by user");
		lastUserState = std::move(newState);
		targetDeviceName = am.getCurrentAudioDevice() != nullptr ? am.getCurrentAudioDevice()->getName() : "";
	}
	else
	{
		//LOG("Audio setup changed (no user action)");

	}

	updateGraph();
}

StringArray AudioManager::getInputChannelNames() const
{
	if (am.getCurrentAudioDevice() == nullptr) return StringArray();

	StringArray allInputs = am.getCurrentAudioDevice()->getInputChannelNames();
	BigInteger actives = am.getCurrentAudioDevice()->getActiveInputChannels();

	StringArray result;
	for (int i = 0; i < allInputs.size(); i++)  if (actives[i]) result.add(allInputs[i]);
	return result;
}

StringArray AudioManager::getOutputChannelNames() const
{
	if (am.getCurrentAudioDevice() == nullptr) return StringArray();

	StringArray allOutputs = am.getCurrentAudioDevice()->getOutputChannelNames();
	BigInteger actives = am.getCurrentAudioDevice()->getActiveOutputChannels();
	StringArray result;
	for (int i = 0; i < allOutputs.size(); i++)  if (actives[i]) result.add(allOutputs[i]);
	return result;
}

void AudioManager::stop()
{
	graph.suspendProcessing(true);
	am.closeAudioDevice();
}

void AudioManager::startLoadFile()
{
	graph.suspendProcessing(true);
}

void AudioManager::endLoadFile()
{
	graph.suspendProcessing(false);
}

String AudioManager::getCurrentDeviceDescription()
{
	String deviceName = am.getCurrentAudioDevice() != nullptr ? am.getCurrentAudioDevice()->getName() : (targetDeviceName.isNotEmpty() ? "[Disconnected :" + targetDeviceName + "]" : "");

	am.getAudioDeviceSetup();

	return deviceName + " : " + String(currentSampleRate) + "Hz, " + String(currentBufferSize) + " samples";
}

var AudioManager::getJSONData(bool includeNonOverriden)
{
	var data = ControllableContainer::getJSONData(includeNonOverriden);
	std::unique_ptr<XmlElement> xmlData(am.createStateXml());
	if (xmlData != nullptr)  data.getDynamicObject()->setProperty("audioSettings", xmlData->toString());

	data.getDynamicObject()->setProperty("deviceName", am.getCurrentAudioDevice() != nullptr ? am.getCurrentAudioDevice()->getName() : "");
	if (am.getCurrentAudioDevice())
	{
		var setupData = var(new DynamicObject());
		AudioDeviceManager::AudioDeviceSetup setup(am.getAudioDeviceSetup());
		setupData.getDynamicObject()->setProperty("deviceType", am.getCurrentAudioDeviceType());
		setupData.getDynamicObject()->setProperty("inputDeviceName", setup.inputDeviceName);
		setupData.getDynamicObject()->setProperty("outputDeviceName", setup.outputDeviceName);
		setupData.getDynamicObject()->setProperty("sampleRate", setup.sampleRate);
		setupData.getDynamicObject()->setProperty("bufferSize", setup.bufferSize);
		setupData.getDynamicObject()->setProperty("inputChannels", setup.inputChannels.toMemoryBlock().toBase64Encoding());
		setupData.getDynamicObject()->setProperty("outputChannels", setup.outputChannels.toMemoryBlock().toBase64Encoding());
		data.getDynamicObject()->setProperty("audioSetup", setupData);
	}

	return data;
}

void AudioManager::loadJSONDataInternal(var data)
{
	String xmlState = data.getProperty("audioSettings", "").toString();
	if (xmlState.isNotEmpty()) lastUserState = XmlDocument::parse(xmlState);

	var audioSetup = data.getProperty("audioSetup", var());
	loadAudioConfig(audioSetup);

	targetDeviceName = data.getProperty("deviceName", "").toString();



	//if (!checkAudioConfig()) targetAudioSetup = data.getProperty("audioSetup", var());
}

InspectableEditor* AudioManager::getEditorInternal(bool isRoot, Array<Inspectable*> inspectables)
{
	return new AudioManagerEditor(this, isRoot);
}
