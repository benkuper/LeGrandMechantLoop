/*
  ==============================================================================

	MIDIIONode.h
	Created: 15 Nov 2020 8:42:29am
	Author:  bkupe

  ==============================================================================
*/

#pragma once

class VSTParameterContainer;

class MIDIInputNode :
	public Node
{
public:
	MIDIInputNode(var params = var());
	~MIDIInputNode();

	void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

	DECLARE_TYPE("MIDI Input");
};

class MIDIOutputNode :
	public Node
{
public:
	MIDIOutputNode(var params = var());
	~MIDIOutputNode();

	void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

	DECLARE_TYPE("MIDI Output");
};
