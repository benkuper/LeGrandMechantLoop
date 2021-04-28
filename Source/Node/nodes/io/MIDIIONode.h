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

	void setMIDIInDevice(MIDIInputDevice* d) override;
	void setMIDIOutDevice(MIDIOutputDevice* d) override;

	String getTypeString() const override { return getTypeStringStatic(); }
	static const String getTypeStringStatic() { return "MIDI IO"; }
};
