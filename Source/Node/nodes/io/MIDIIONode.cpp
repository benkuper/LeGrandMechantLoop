/*
  ==============================================================================

	MIDIIONode.cpp
	Created: 15 Nov 2020 8:42:29am
	Author:  bkupe

  ==============================================================================
*/

#include "Node/NodeIncludes.h"

MIDIIONode::MIDIIONode(var params) :
	Node(getTypeString(), params, true, true, false, false, true, true)
{
	setAudioInputs(1, false);
	setAudioOutputs(2);

	viewUISize->setPoint(200, 100);
}

MIDIIONode::~MIDIIONode()
{

}

void MIDIIONode::midiMessageReceived(MIDIInterface* i, const MidiMessage& m)
{
	//Node::midiMessageReceived(m);

	if (!enabled->boolValue()) return;
	if (m.getChannel() > 0 && !midiChannels[m.getChannel() - 1]->boolValue()) return;

	if (logIncomingMidi->boolValue())
	{
		NLOG(niceName, "Received MIDI : " << m.getDescription());
	}

	for (auto& c : outMidiConnections) c->destNode->midiMessageReceived(i, m);

}

void MIDIIONode::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
	if (midiInterface != nullptr && midiInterface->outputDevice != nullptr)
	{
		for (const auto m : midiMessages)
		{
			MidiMessage msg = m.getMessage();
			midiInterface->outputDevice->sendMessage(msg);
		}
	}
}