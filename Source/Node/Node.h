/*
  ==============================================================================

    Node.h
    Created: 15 Nov 2020 8:40:03am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"

class NodeAudioProcessor;

class Node :
    public BaseItem
{
public:
    enum Type {CUSTOM, INPUT, OUTPUT, LOOPER, MIXER, SPAT, VST};

    Node(StringRef name = "Node", Type type = CUSTOM, NodeAudioProcessor * processor = nullptr, var params = var());
    virtual ~Node();

    virtual void clearItem() override;

    Type type;

    AudioProcessorGraph::NodeID graphNodeID;
    NodeAudioProcessor* audioProcessor;
    AudioProcessorGraph::Node::Ptr nodeGraphPtr;

    StringArray audioInputNames;
    StringArray audioOutputNames;

    FloatParameter* outGain;

    void setAudioInputs(const StringArray & inputNames);
    void setAudioOutputs(const StringArray& outputNames);

    // Helpers
    bool hasAudioInput() { return !audioInputNames.isEmpty(); }
    bool hasAudioOutput() { return !audioOutputNames.isEmpty(); }
};

class NodeAudioProcessor :
    public AudioProcessor
{
public:
    NodeAudioProcessor(Node* n);
    virtual ~NodeAudioProcessor();

    Node* node;

    virtual void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) {}

    /* AudioProcessor */
    virtual const String getName() const override { return node == nullptr ? "[Deleted]" : node->niceName; }
    virtual void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) {}
    virtual void releaseResources() override {}
    virtual void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;
    virtual double getTailLengthSeconds() const override { return 0; }
    virtual bool acceptsMidi() const override { return false; }
    virtual bool producesMidi() const override { return false; }

    virtual AudioProcessorEditor* createEditor() override { return nullptr; }
    virtual bool hasEditor() const override { return false; }
    virtual int getNumPrograms() override { return 0; }
    virtual int getCurrentProgram() override { return 0; }
    virtual void setCurrentProgram(int index) override {}
    virtual const String getProgramName(int index) override { return "[NoProgram]"; }
    virtual void changeProgramName(int index, const String& newName) override {}
    virtual void getStateInformation(juce::MemoryBlock& destData) override {}
    virtual void setStateInformation(const void* data, int sizeInBytes) override {}
};