/*
  ==============================================================================

	MIDIIONode.h
	Created: 15 Nov 2020 8:42:29am
	Author:  bkupe

  ==============================================================================
*/

#pragma once
#include "../../Node.h"
#include "Common/MIDI/MIDIDeviceParameter.h"

class VSTParameterContainer;

class MIDIIONode :
	public Node,
	public MIDIInputDevice::MIDIInputListener
{
public:
	MIDIIONode(var params = var());
	~MIDIIONode();

	MIDIDeviceParameter* midiParam;

	MIDIInputDevice* currentInDevice;
	MIDIOutputDevice* currentOutDevice;
	MidiMessageCollector midiCollector;

	void clearItem() override;

	void setMIDIInDevice(MIDIInputDevice* d);
	void setMIDIOutDevice(MIDIOutputDevice* d);
	
	void setIOFromDevices();

	void onContainerParameterChangedInternal(Parameter* p) override;

	void midiMessageReceived(const MidiMessage& m) override;

	void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
	void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;
	void processBlockBypassed(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;
	void processMIDIIO(int numSamples, MidiBuffer& midiMessages, bool bypassed);

	String getTypeString() const override { return getTypeStringStatic(); }
	static const String getTypeStringStatic() { return "MIDI IO"; }
};
