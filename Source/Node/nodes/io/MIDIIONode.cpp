/*
  ==============================================================================

	MIDIIONode.cpp
	Created: 15 Nov 2020 8:42:29am
	Author:  bkupe

  ==============================================================================
*/

#include "Node/NodeIncludes.h"

MIDIIONode::MIDIIONode(var params) :
	Node(getTypeString(), params, true, true, false, false, true, true),
	clock(true)
{
	setAudioInputs(1, false);
	setAudioOutputs(2);

	autoFeedback = midiCC->addBoolParameter("Auto feedback", "If auto feedback, this will send received message to output", false);
	enableClock = midiCC->addBoolParameter("Send Clock", "If checked, this will send the clock to the connected output device", true);
	viewUISize->setPoint(200, 100);
}

MIDIIONode::~MIDIIONode()
{

}

//void MIDIIONode::setMIDIInDevice(MIDIInputDevice* d)
//{
//	Node::setMIDIInDevice(d);
//}
//
//void MIDIIONode::setMIDIOutDevice(MIDIOutputDevice* d)
//{
//	if (currentOutDevice != nullptr)
//	{
//	}
//
//	clock.setOutput(nullptr);
//
//	Node::setMIDIOutDevice(d);
//
//	if (currentOutDevice != nullptr)
//	{
//		if(enabled->boolValue() && enableClock->boolValue()) clock.setOutput(currentOutDevice);
//	}
//}

void MIDIIONode::setMIDIInterface(MIDIInterface* i)
{
	Node::setMIDIInterface(i);

	clock.setOutput(nullptr);

	if (enabled->boolValue() && midiInterface != nullptr && midiInterface->outputDevice != nullptr)
	{
		clock.setOutput(enabled->boolValue() && enableClock->boolValue() ? midiInterface->outputDevice : nullptr);
	}
}

void MIDIIONode::addInConnection(NodeConnection* c)
{
	Node::addInConnection(c);

}

void MIDIIONode::removeInConnection(NodeConnection* c)
{
	Node::removeInConnection(c);
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

	if (autoFeedback->boolValue() && midiInterface != nullptr && midiInterface->outputDevice != nullptr) midiInterface->outputDevice->sendMessage(m);
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

void MIDIIONode::onContainerParameterChangedInternal(Parameter* p)
{
	Node::onContainerParameterChangedInternal(p);

	if (p == enabled)
	{
		if (midiInterface != nullptr && midiInterface->outputDevice != nullptr)
		{
			clock.setOutput(enabled->boolValue() && enableClock->boolValue() ? midiInterface->outputDevice : nullptr);
		}
	}
}

void MIDIIONode::onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c)
{
	Node::onControllableFeedbackUpdateInternal(cc, c);

	if (c == enableClock)
	{
		if (midiInterface != nullptr && midiInterface->outputDevice != nullptr)
		{
			clock.setOutput(enabled->boolValue() && enableClock->boolValue() ? midiInterface->outputDevice : nullptr);
		}
	}
}
