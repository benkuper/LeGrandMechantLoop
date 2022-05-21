/*
  ==============================================================================

	MIDIIONode.cpp
	Created: 15 Nov 2020 8:42:29am
	Author:  bkupe

  ==============================================================================
*/

MIDIIONode::MIDIIONode(var params) :
	Node(getTypeString(), params, true, true, false, false, true, true),
	clock(true)
{
	setAudioInputs(1, false);
	setAudioOutputs(2);

	enableClock = midiCC->addBoolParameter("Send Clock", "If checked, this will send the clock to the connected output device", true);
	viewUISize->setPoint(200, 100);
}

MIDIIONode::~MIDIIONode()
{

}

void MIDIIONode::setMIDIInDevice(MIDIInputDevice* d)
{
	Node::setMIDIInDevice(d);
}

void MIDIIONode::setMIDIOutDevice(MIDIOutputDevice* d)
{
	if (currentOutDevice != nullptr)
	{
	}

	clock.setOutput(nullptr);

	Node::setMIDIOutDevice(d);

	if (currentOutDevice != nullptr)
	{
		if(enabled->boolValue() && enableClock->boolValue()) clock.setOutput(currentOutDevice);
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

void MIDIIONode::midiMessageReceived(const MidiMessage& m)
{
	//Node::midiMessageReceived(m);

	if (!enabled->boolValue()) return;
	if (m.getChannel() > 0 && !midiChannels[m.getChannel() - 1]->boolValue()) return;

	if (logIncomingMidi->boolValue())
	{
		NLOG(niceName, "Received MIDI : " << m.getDescription());
	}

	for (auto& c : outMidiConnections) c->destNode->midiMessageReceived(m);
}

void MIDIIONode::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
	if (currentOutDevice != nullptr)
	{
		for (auto& m : midiMessages) currentOutDevice->sendMessage(m.getMessage());
	}
}

void MIDIIONode::onContainerParameterChangedInternal(Parameter* p)
{
	Node::onContainerParameterChangedInternal(p);

	if (p == enabled)
	{
		if (currentOutDevice != nullptr) clock.setOutput(enabled->boolValue() && enableClock->boolValue() ? currentOutDevice : nullptr);
	}
}

void MIDIIONode::onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c)
{
	Node::onControllableFeedbackUpdateInternal(cc, c);

	if (c == enableClock)
	{
		if (currentOutDevice != nullptr) clock.setOutput(enabled->boolValue() && enableClock->boolValue() ? currentOutDevice : nullptr);
	}
}
