/*
  ==============================================================================

	NodeAudioProcessor.h
	Created: 15 Nov 2020 10:34:19pm
	Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"

class Node;
class NodeViewUI;

class NodeAudioProcessor :
	public ControllableContainer,
	public AudioProcessor
{
public:
	NodeAudioProcessor(Node* n, bool useOutRMS = true);
	virtual ~NodeAudioProcessor() {}

	WeakReference<Node> nodeRef;

	FloatParameter* outRMS;

	virtual bool exposeAudioInput() { return getExpectedNumInputs() > 0; }
	virtual bool exposeAudioOutput() { return getExpectedNumOutputs() > 0; }

	virtual void updateInputsFromNode();
	virtual void updateInputsFromNodeInternal() {}
	virtual void updateOutputsFromNode();
	virtual void updateOutputsFromNodeInternal() {}

	virtual int getExpectedNumInputs();
	virtual int getExpectedNumOutputs();
	virtual void updatePlayConfig();

	virtual void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) {}

	/* AudioProcessor */
	virtual const String getName() const override;
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



class GenericNodeAudioProcessor :
	public NodeAudioProcessor
{
public:
	GenericNodeAudioProcessor(Node* n, bool useOutRMS = true);
	virtual ~GenericNodeAudioProcessor() {}


	virtual NodeViewUI* createNodeViewUI(); //to override by children
};
