/*
  ==============================================================================

	MIDIInputNode.cpp
	Created: 15 Nov 2020 8:42:29am
	Author:  bkupe

  ==============================================================================
*/

#include "Node/NodeIncludes.h"

MIDIInputNode::MIDIInputNode(var params) :
	Node(getTypeString(), params, false, false, false, false, true, false)
{
	setMIDIIO(false, true);

	viewUISize->setPoint(200, 100);
}

MIDIInputNode::~MIDIInputNode()
{

}

void MIDIInputNode::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
	midiMessages.clear();
	Node::processBlock(buffer, midiMessages);
}

MIDIOutputNode::MIDIOutputNode(var params) :
	Node(getTypeString(), params, false, false, false, false, false, true)
{
	setMIDIIO(true, false);
}

MIDIOutputNode::~MIDIOutputNode()
{
}

void MIDIOutputNode::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
	Node::processBlock(buffer, midiMessages);

	if (!midiMessages.isEmpty()) return;
	if (midiInterface == nullptr || midiInterface->outputDevice == nullptr) return;
	if (!midiInterface->enabled->boolValue()) return;
	
	midiInterface->outputDevice->device->sendBlockOfMessagesNow(midiMessages);

	midiMessages.clear();
}
