/*

  ==============================================================================



	AudioManager.cpp

	Created: 15 Nov 2020 8:44:58am

	Author:  bkupe



  ==============================================================================

*/



#include "ui/AudioManagerEditor.h"

#include "Transport/Transport.h"



namespace
{

	constexpr double preferredSampleRate = 48000.0;

	constexpr int preferredBufferSize = 128;

	constexpr int preferredNumOutputChannels = 2;

}



juce_ImplementSingleton(AudioManager)



AudioManager::AudioManager() :

	ControllableContainer("Audio Settings"),

	graphIDIncrement(GRAPH_START_ID),

	hasPreferredSetup(false),

	isApplyingManagedChange(false)

	//;isSettingUp(false)

{

	showWarningInUI = true;



	am.addAudioCallback(this);

	am.addChangeListener(this);

	am.initialiseWithDefaultDevices(0, 2);



	AudioDeviceManager::AudioDeviceSetup setup(am.getAudioDeviceSetup());

	setup.sampleRate = preferredSampleRate;

	setup.bufferSize = preferredBufferSize;

	setup.useDefaultInputChannels = false;

	setup.useDefaultOutputChannels = true;



	//am.setAudioDeviceSetup(setup, false);



	am.addAudioCallback(&player);



	graph.reset();



	setup = am.getAudioDeviceSetup();

	currentSampleRate = setup.sampleRate;

	currentBufferSize = setup.bufferSize;

	capturePreferredConfiguration();



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



AudioDeviceManager::AudioDeviceSetup AudioManager::createPreferredManualSetup() const

{

	AudioDeviceManager::AudioDeviceSetup setup(am.getAudioDeviceSetup());

	setup.sampleRate = preferredSampleRate;

	setup.bufferSize = preferredBufferSize;

	setup.useDefaultOutputChannels = false;

	setup.outputChannels.clear();



	if (auto* device = am.getCurrentAudioDevice())

	{

		const int numOutputs = device->getOutputChannelNames().size();

		for (int i = 0; i < jmin(preferredNumOutputChannels, numOutputs); ++i) setup.outputChannels.setBit(i);

	}



	return setup;

}



AudioDeviceManager::AudioDeviceSetup AudioManager::createDisabledSetup() const

{

	AudioDeviceManager::AudioDeviceSetup setup(hasPreferredSetup ? preferredSetup : am.getAudioDeviceSetup());

	setup.inputDeviceName = "";

	setup.outputDeviceName = "";

	setup.useDefaultInputChannels = false;

	setup.useDefaultOutputChannels = false;

	setup.inputChannels.clear();

	setup.outputChannels.clear();

	return setup;

}



AudioDeviceManager::AudioDeviceSetup AudioManager::getSetupFromConfig(const var& config) const

{

	AudioDeviceManager::AudioDeviceSetup setup(am.getAudioDeviceSetup());

	if (!config.isObject()) return setup;



	setup.inputDeviceName = config.getProperty("inputDeviceName", "").toString();

	setup.outputDeviceName = config.getProperty("outputDeviceName", "").toString();

	setup.sampleRate = config.getProperty("sampleRate", preferredSampleRate).toString().getDoubleValue();

	setup.bufferSize = config.getProperty("bufferSize", preferredBufferSize).toString().getIntValue();

	setup.useDefaultInputChannels = false;

	setup.useDefaultOutputChannels = false;



	BigInteger inputChannels;

	MemoryBlock inputBlock;

	inputBlock.fromBase64Encoding(config.getProperty("inputChannels", "").toString());

	inputChannels.loadFromMemoryBlock(inputBlock);



	BigInteger outputChannels;

	MemoryBlock outputBlock;

	outputBlock.fromBase64Encoding(config.getProperty("outputChannels", "").toString());

	outputChannels.loadFromMemoryBlock(outputBlock);



	setup.inputChannels = inputChannels;

	setup.outputChannels = outputChannels;

	return setup;

}



void AudioManager::capturePreferredConfiguration()

{

	preferredSetup = am.getAudioDeviceSetup();

	preferredDeviceType = am.getCurrentAudioDeviceType();

	hasPreferredSetup = true;

	targetDeviceName = getSetupDeviceName(preferredSetup);



	if (std::unique_ptr<XmlElement> newState = am.createStateXml()) lastUserState = std::move(newState);

}



bool AudioManager::isCurrentSetupMatchingPreferred() const

{

	return isSetupMatchingPreferred(am.getAudioDeviceSetup(), am.getCurrentAudioDeviceType());

}



bool AudioManager::isSetupMatchingPreferred(const AudioDeviceManager::AudioDeviceSetup& setup, const String& deviceType) const

{

	if (!hasPreferredSetup) return false;

	const bool typeMatches = preferredDeviceType.isEmpty() || deviceType == preferredDeviceType;

	return typeMatches && setup == preferredSetup;

}



bool AudioManager::isPreferredConfigurationAvailable()

{

	if (!hasPreferredSetup) return false;

	if (preferredSetup.inputDeviceName.isEmpty() && preferredSetup.outputDeviceName.isEmpty()) return true;



	for (auto& deviceType : am.getAvailableDeviceTypes())

	{

		const StringArray deviceNames = deviceType->getDeviceNames();

		const bool inputFound = preferredSetup.inputDeviceName.isEmpty() || deviceNames.contains(preferredSetup.inputDeviceName);

		const bool outputFound = preferredSetup.outputDeviceName.isEmpty() || deviceNames.contains(preferredSetup.outputDeviceName);

		if (inputFound && outputFound) return true;

	}



	return false;

}



bool AudioManager::hasActiveOutputDevice() const

{

	if (auto* device = am.getCurrentAudioDevice()) return device->getActiveOutputChannels().countNumberOfSetBits() > 0;

	return false;

}



void AudioManager::applyPreferredConfigurationIfAvailable()

{

	if (!hasPreferredSetup || isCurrentSetupMatchingPreferred()) return;



	isApplyingManagedChange = true;



	if (lastUserState != nullptr)

	{

		loadAudioConfig();

	}

	else

	{

		if (preferredDeviceType.isNotEmpty() && am.getCurrentAudioDeviceType() != preferredDeviceType) am.setCurrentAudioDeviceType(preferredDeviceType, false);



		const String error = am.setAudioDeviceSetup(preferredSetup, false);

		if (error.isNotEmpty()) setWarningMessage("Could not restore preferred audio settings: " + error, "device");

		else clearWarning("device");

	}



	isApplyingManagedChange = false;

}



void AudioManager::disableAudioDeviceTemporarily()

{

	const AudioDeviceManager::AudioDeviceSetup disabledSetup = createDisabledSetup();

	if (am.getAudioDeviceSetup() == disabledSetup) return;



	isApplyingManagedChange = true;

	const String error = am.setAudioDeviceSetup(disabledSetup, false);

	isApplyingManagedChange = false;



	if (error.isNotEmpty()) setWarningMessage("Could not disable audio device: " + error, "device");

}



bool AudioManager::areXmlStatesEquivalent(const XmlElement* first, const XmlElement* second)

{

	if (first == nullptr || second == nullptr) return first == second;

	return first->isEquivalentTo(second, true);

}



String AudioManager::getSetupDeviceName(const AudioDeviceManager::AudioDeviceSetup& setup)

{

	if (setup.outputDeviceName.isNotEmpty()) return setup.outputDeviceName;

	return setup.inputDeviceName;

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

		String deviceType = fallbackConfig.getProperty("deviceType", preferredDeviceType).toString();

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

			setup = getSetupFromConfig(fallbackConfig);

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

	if (isApplyingManagedChange) return;



	if (hasPreferredSetup && !isCurrentSetupMatchingPreferred() && isPreferredConfigurationAvailable()) applyPreferredConfigurationIfAvailable();

}



void AudioManager::changeListenerCallback(ChangeBroadcaster* source)

{

	ignoreUnused(source);



	if (isApplyingManagedChange)

	{

		updateGraph();

		return;

	}



	const AudioDeviceManager::AudioDeviceSetup currentSetup(am.getAudioDeviceSetup());

	const String currentDeviceType = am.getCurrentAudioDeviceType();

	std::unique_ptr<XmlElement> newState = am.createStateXml();

	const bool preferredStateChanged = !areXmlStatesEquivalent(lastUserState.get(), newState.get());

	const bool deviceSelectionChanged = !hasPreferredSetup

		|| currentDeviceType != preferredDeviceType

		|| currentSetup.inputDeviceName != preferredSetup.inputDeviceName

		|| currentSetup.outputDeviceName != preferredSetup.outputDeviceName;



	if (preferredStateChanged)

	{

		if (deviceSelectionChanged)

		{

			const AudioDeviceManager::AudioDeviceSetup preferredManualSetup = createPreferredManualSetup();

			if (!(preferredManualSetup == currentSetup))

			{

				isApplyingManagedChange = true;

				const String error = am.setAudioDeviceSetup(preferredManualSetup, true);

				isApplyingManagedChange = false;



				if (error.isNotEmpty()) setWarningMessage("Could not apply preferred audio settings: " + error, "device");

				else clearWarning("device");

			}

		}



		capturePreferredConfiguration();

		updateGraph();

		return;

	}



	if (hasPreferredSetup && !isSetupMatchingPreferred(currentSetup, currentDeviceType))

	{

		if (isPreferredConfigurationAvailable())

		{

			applyPreferredConfigurationIfAvailable();

		}

		else if (hasActiveOutputDevice())

		{

			disableAudioDeviceTemporarily();

		}



		updateGraph();

		return;

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



	data.getDynamicObject()->setProperty("deviceName", targetDeviceName);

	if (hasPreferredSetup)

	{

		var setupData = var(new DynamicObject());

		setupData.getDynamicObject()->setProperty("deviceType", preferredDeviceType);

		setupData.getDynamicObject()->setProperty("inputDeviceName", preferredSetup.inputDeviceName);

		setupData.getDynamicObject()->setProperty("outputDeviceName", preferredSetup.outputDeviceName);

		setupData.getDynamicObject()->setProperty("sampleRate", preferredSetup.sampleRate);

		setupData.getDynamicObject()->setProperty("bufferSize", preferredSetup.bufferSize);

		setupData.getDynamicObject()->setProperty("inputChannels", preferredSetup.inputChannels.toMemoryBlock().toBase64Encoding());

		setupData.getDynamicObject()->setProperty("outputChannels", preferredSetup.outputChannels.toMemoryBlock().toBase64Encoding());

		data.getDynamicObject()->setProperty("audioSetup", setupData);

	}



	return data;

}



void AudioManager::loadJSONDataInternal(var data)

{

	String xmlState = data.getProperty("audioSettings", "").toString();

	if (xmlState.isNotEmpty()) lastUserState = XmlDocument::parse(xmlState);



	var audioSetup = data.getProperty("audioSetup", var());

	if (audioSetup.isObject())

	{

		preferredSetup = getSetupFromConfig(audioSetup);

		preferredDeviceType = audioSetup.getProperty("deviceType", "").toString();

		hasPreferredSetup = true;

		targetDeviceName = getSetupDeviceName(preferredSetup);

	}

	else

	{

		hasPreferredSetup = false;

		targetDeviceName = data.getProperty("deviceName", "").toString();

	}



	loadAudioConfig(audioSetup);



	if (!hasPreferredSetup && am.getCurrentAudioDevice() != nullptr) capturePreferredConfiguration();



	//if (!checkAudioConfig()) targetAudioSetup = data.getProperty("audioSetup", var());

}



InspectableEditor* AudioManager::getEditorInternal(bool isRoot, Array<Inspectable*> inspectables)

{

	return new AudioManagerEditor(this, isRoot);

}
