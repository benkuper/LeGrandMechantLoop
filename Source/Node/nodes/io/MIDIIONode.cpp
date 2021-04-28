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
		clock.setOutput(nullptr);
	}

	Node::setMIDIOutDevice(d);

	if (currentOutDevice != nullptr)
	{
		clock.setOutput(currentOutDevice);
	}
}