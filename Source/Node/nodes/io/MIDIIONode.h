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

	void midiMessageReceived(MIDIInterface* i, const MidiMessage& m) override;

	void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

	DECLARE_TYPE("MIDI IO");
};
