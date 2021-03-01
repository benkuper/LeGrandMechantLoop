/*
  ==============================================================================

    AudioManager.h
    Created: 15 Nov 2020 8:44:58am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"

#define AUDIO_GRAPH_INPUT_ID 1
#define AUDIO_GRAPH_OUTPUT_ID 2


class AudioManager :
    public ControllableContainer, 
	public AudioIODeviceCallback,
	public ChangeListener,
	public EngineListener
{
public:
	juce_DeclareSingleton(AudioManager, true);
	
    AudioManager();
    ~AudioManager();

	AudioDeviceManager am;
	AudioProcessorGraph graph;
	AudioProcessorPlayer player;
	int graphIDIncrement; //This will be incremented and assign to each node that is created, in the node constructor

	double currentSampleRate;
	int currentBufferSize;

	int numAudioInputs;
	int numAudioOutputs;


	int getNewGraphID();

	void updateGraph();

	virtual void audioDeviceIOCallback(const float** inputChannelData, int numInputChannels, float** outputChannelData, int numOutputChannels, int numSamples) override;
	virtual void audioDeviceAboutToStart(AudioIODevice* device) override;
	virtual void audioDeviceStopped() override;

	virtual void changeListenerCallback(ChangeBroadcaster* source) override;

	StringArray getInputChannelNames() const;
	StringArray getOutputChannelNames() const;

	void startLoadFile() override;
	void endLoadFile() override;

	class AudioManagerListener
	{
	public:
		virtual ~AudioManagerListener() {}
		virtual void audioSetupChanged() {}
	};

	ListenerList<AudioManagerListener> audioManagerListeners;
	void addAudioManagerListener(AudioManagerListener* newListener) { audioManagerListeners.add(newListener); }
	void removeAudioManagerListener(AudioManagerListener* listener) { audioManagerListeners.remove(listener); }

	var getJSONData() override;
	void loadJSONDataInternal(var data) override;

	InspectableEditor* getEditor(bool isRoot) override;
};