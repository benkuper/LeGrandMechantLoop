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
    am.initialiseWithDefaultDevices(1, 2);

    graph.reset();

    AudioDeviceManager::AudioDeviceSetup setup = am.getAudioDeviceSetup();
    currentSampleRate = setup.sampleRate;
    currentBufferSize = setup.bufferSize;

    graph.setPlayConfigDetails(1, 2, currentSampleRate, currentBufferSize);
    graph.prepareToPlay(currentSampleRate, currentBufferSize);

    std::unique_ptr<AudioProcessorGraph::AudioGraphIOProcessor> procIn(new AudioProcessorGraph::AudioGraphIOProcessor(AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode));
    std::unique_ptr<AudioProcessorGraph::AudioGraphIOProcessor> procOut(new AudioProcessorGraph::AudioGraphIOProcessor(AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode));
    graph.addNode(std::move(procIn), AudioProcessorGraph::NodeID(AUDIO_GRAPH_INPUT_ID));
    graph.addNode(std::move(procOut), AudioProcessorGraph::NodeID(AUDIO_GRAPH_OUTPUT_ID));
}

AudioManager::~AudioManager()
{
    graph.clear();
    am.removeAudioCallback(this);
    am.removeChangeListener(this);
}

int AudioManager::getNewGraphID()
{
    return graphIDIncrement++;
}

void AudioManager::audioDeviceIOCallback(const float** inputChannelData, int numInputChannels, float** outputChannelData, int numOutputChannels, int numSamples)
{
	for (int i = 0; i < numOutputChannels; ++i) FloatVectorOperations::clear(outputChannelData[i], numSamples);
}

void AudioManager::audioDeviceAboutToStart(AudioIODevice* device)
{
}

void AudioManager::audioDeviceStopped()
{
}

void AudioManager::changeListenerCallback(ChangeBroadcaster* source)
{
    AudioDeviceManager::AudioDeviceSetup setup = am.getAudioDeviceSetup();
    currentSampleRate = setup.sampleRate;
    currentBufferSize = setup.bufferSize;

    int numSelectedInputChannelsInSetup = setup.inputChannels.countNumberOfSetBits();
    int numSelectedOutputChannelsInSetup = setup.outputChannels.countNumberOfSetBits();

    graph.setPlayConfigDetails(numSelectedInputChannelsInSetup, numSelectedOutputChannelsInSetup, currentSampleRate, currentBufferSize);
    graph.prepareToPlay(currentSampleRate, currentBufferSize);
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
        am.initialise(0, 2, elem.get(), true);
    }
}

InspectableEditor* AudioManager::getEditor(bool isRoot)
{
    return new AudioManagerEditor(this, isRoot);
}
