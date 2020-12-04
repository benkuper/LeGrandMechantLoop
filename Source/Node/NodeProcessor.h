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
	NodeProcessor(Node* n, bool hasInput = true, bool hasOutput = true, bool userCanSetIO = false, bool useOutControl = true);

	virtual ~NodeProcessor() {}

	WeakReference<Node> nodeRef;
	
	bool hasAudioInput; //defines if this processor is supposed to get input, even if the number of channels is zero
	bool hasAudioOutput; //defines if this processor is supposed to provide output, even if the number of channels is zero
	bool hasMIDIInput;
	bool hasMIDIOutput;

	IntParameter * numAudioInputs;
	IntParameter * numAudioOutputs;

	Array<float> connectionsActivityLevels;

	FloatParameter* outRMS;
	FloatParameter* outGain;

	MidiBuffer inMidiBuffer;

	virtual void clearProcessor() {}

	virtual void init();
	virtual void initInternal() {}

	virtual void onContainerParameterChanged(Parameter* p) override;
	
	virtual void autoSetNumInputs(); //if userCanSetIO
	virtual void autoSetNumOutputs(); //if userCanSetIO

	virtual void updateInputsFromNode(bool updatePlayConfig = true);
	virtual void updateInputsFromNodeInternal() {}
	virtual void updateOutputsFromNode(bool updatePlayConfig = true);
	virtual void updateOutputsFromNodeInternal() {}

	virtual void updatePlayConfig();

	void receiveMIDIFromInput(Node* n, MidiBuffer& inputBuffer);
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
	GenericNodeProcessor(Node* n, bool hasInput = true, bool hasOutput = true, bool userCanSetIO = false, bool useOutControl = true);
	virtual ~GenericNodeProcessor() {}

	virtual NodeViewUI* createNodeViewUI(); //to override by children
};
