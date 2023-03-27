/*
  ==============================================================================

	MIDIIONode.h
	Created: 15 Nov 2020 8:42:29am
	Author:  bkupe

  ==============================================================================
*/

#pragma once

class VSTParameterContainer;

class MIDIIONode :
	public Node
{
public:
	MIDIIONode(var params = var());
	~MIDIIONode();

	MIDIClock clock;
	BoolParameter* enableClock;
	BoolParameter* autoFeedback;

	//void setMIDIInDevice(MIDIInputDevice* d) override;
	//void setMIDIOutDevice(MIDIOutputDevice* d) override;

	void setMIDIInterface(MIDIInterface* i) override;

	void addInConnection(NodeConnection* c) override;
	void removeInConnection(NodeConnection* c) override;

	void midiMessageReceived(MIDIInterface* i, const MidiMessage& m) override;

	void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

	void onContainerParameterChangedInternal(Parameter* p) override;
	void onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c) override;

	String getTypeString() const override { return getTypeStringStatic(); }
	static const String getTypeStringStatic() { return "MIDI IO"; }
};
