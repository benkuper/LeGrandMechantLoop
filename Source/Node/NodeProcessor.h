/*
  ==============================================================================

	NodeProcessor.h
	Created: 15 Nov 2020 10:34:19pm
	Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"

class Node;
class NodeViewUI;

class NodeProcessor :
	public ControllableContainer,
	public AudioProcessor
{
public:
	NodeProcessor(Node* n, bool hasInput = true, bool hasOutput = true, bool useOutRMS = true);
	virtual ~NodeProcessor() {}

	WeakReference<Node> nodeRef;

	int hasInput; //defines if this processor is supposed to get input, even if the number of channels is zero
	int hasOutput; //defines if this processor is supposed to provide output, even if the number of channels is zero

	FloatParameter* outRMS;

	virtual void updateInputsFromNode(bool updatePlayConfig = true);
	virtual void updateInputsFromNodeInternal() {}
	virtual void updateOutputsFromNode(bool updatePlayConfig = true);
	virtual void updateOutputsFromNodeInternal() {}

	virtual void updatePlayConfig();

	virtual void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) {}

	/* AudioProcessor */
	virtual const String getName() const override;
	virtual void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override {}
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



class GenericNodeProcessor :
	public NodeProcessor
{
public:
	GenericNodeProcessor(Node* n, bool hasInput = true, bool hasOutput =true, bool useOutRMS = true);
	virtual ~GenericNodeProcessor() {}


	virtual NodeViewUI* createNodeViewUI(); //to override by children
};
