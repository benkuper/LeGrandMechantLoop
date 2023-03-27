/*
  ==============================================================================

    AudioManager.cpp
    Created: 15 Nov 2020 8:44:58am
    Author:  bkupe

  ==============================================================================
*/

#include "AudioManager.h"
#include "ui/AudioManagerEditor.h"

juce_ImplementSingleton(AudioManager)

AudioManager::AudioManager() :
    ControllableContainer("Audio Settings"),
    graphIDIncrement(3)
{
    am.addAudioCallback(this);
    am.addChangeListener(this);
    am.initialiseWithDefaultDevices(2, 2);

    am.addAudioCallback(&player);

    graph.reset();

    AudioDeviceManager::AudioDeviceSetup setup = am.getAudioDeviceSetup();
    currentSampleRate = setup.sampleRate;
    currentBufferSize = setup.bufferSize;

    
    std::unique_ptr<AudioProcessorGraph::AudioGraphIOProcessor> procIn(new AudioProcessorGraph::AudioGraphIOProcessor(AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode));
    std::unique_ptr<AudioProcessorGraph::AudioGraphIOProcessor> procOut(new AudioProcessorGraph::AudioGraphIOProcessor(AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode));
    graph.addNode(std::move(procIn), AudioProcessorGraph::NodeID(AUDIO_GRAPH_INPUT_ID));
    graph.addNode(std::move(procOut), AudioProcessorGraph::NodeID(AUDIO_GRAPH_OUTPUT_ID));

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

void AudioManager::audioDeviceAboutToStart(AudioIODevice* device)
{
}

void AudioManager::audioDeviceStopped()
{
}

void AudioManager::changeListenerCallback(ChangeBroadcaster* source)
{
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

void AudioManager::startLoadFile()
{
    graph.suspendProcessing(true);
}

void AudioManager::endLoadFile()
{
    graph.suspendProcessing(false);
}

var AudioManager::getJSONData()
{
    var data = ControllableContainer::getJSONData();
    std::unique_ptr<XmlElement> xmlData(am.createStateXml());
    if (xmlData != nullptr)  data.getDynamicObject()->setProperty("audioSettings", xmlData->toString());
    return data;
}

void AudioManager::loadJSONDataInternal(var data)
{
    if (data.getDynamicObject()->hasProperty("audioSettings"))
    {
        std::unique_ptr<XmlElement> elem = XmlDocument::parse(data.getProperty("audioSettings", ""));
        String result = am.initialise(0, 2, elem.get(), false);
        if (result.isNotEmpty())
        {
            LOGERROR("Error loading audio settings, please check your audio settings in File > Project Settings");
        }
    }
}

InspectableEditor* AudioManager::getEditorInternal(bool isRoot, Array<Inspectable*> inspectables)
{
    return new AudioManagerEditor(this, isRoot);
}
