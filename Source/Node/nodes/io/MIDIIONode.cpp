/*
  ==============================================================================

	MIDIIONode.cpp
	Created: 15 Nov 2020 8:42:29am
	Author:  bkupe

  ==============================================================================
*/

MIDIIONode::MIDIIONode(var params) :
	Node(getTypeString(), params, false, false, false, false, true, true),
	clock(true)
{
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
