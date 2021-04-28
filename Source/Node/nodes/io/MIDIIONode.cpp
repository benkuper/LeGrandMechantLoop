/*
  ==============================================================================

	MIDIIONode.cpp
	Created: 15 Nov 2020 8:42:29am
	Author:  bkupe

  ==============================================================================
*/

MIDIIONode::MIDIIONode(var params) :
	Node(getTypeString(), params, false, false, false, false),
	currentInDevice(nullptr),
	currentOutDevice(nullptr),
	clock(true)
{
	midiParam = new MIDIDeviceParameter("MIDI Device", true, true);
	ControllableContainer::addParameter(midiParam);
	setMIDIIO(true, true);

	viewUISize->setPoint(200, 150);
}

MIDIIONode::~MIDIIONode()
{

}

void MIDIIONode::clearItem()
{
	Node::clearItem();
	setMIDIInDevice(nullptr);
	setMIDIOutDevice(nullptr);
}

void MIDIIONode::setMIDIInDevice(MIDIInputDevice* d)
{
	if (currentInDevice == d) return;
	if (currentInDevice != nullptr)
	{
		currentInDevice->removeMIDIInputListener(this);
	}

	currentInDevice = d;

	if (currentInDevice != nullptr)
	{
		currentInDevice->addMIDIInputListener(this);
	}
	setIOFromDevices();
}

void MIDIIONode::setMIDIOutDevice(MIDIOutputDevice* d)
{
	if (currentOutDevice == d) return;
	if (currentOutDevice != nullptr)
	{
		currentOutDevice->close();
		clock.setOutput(nullptr);
	}
	currentOutDevice = d;

	if (currentOutDevice != nullptr)
	{
		currentOutDevice->open();
		clock.setOutput(currentOutDevice);
	}

	setIOFromDevices();
}

void MIDIIONode::setIOFromDevices()
{
	//
}

void MIDIIONode::onContainerParameterChangedInternal(Parameter* p)
{
	Node::onContainerParameterChangedInternal(p);
	if (p == midiParam)
	{
		setMIDIInDevice(midiParam->inputDevice);
		setMIDIOutDevice(midiParam->outputDevice);
	}
}

void MIDIIONode::midiMessageReceived(const MidiMessage& m)
{
	if (!enabled->boolValue()) return;
	midiCollector.addMessageToQueue(m); //ugly hack to have at least events sorted, but sampleNumber should be exact
}

void MIDIIONode::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock)
{
	if (sampleRate != 0) midiCollector.reset(sampleRate);
}

void MIDIIONode::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
	processMIDIIO(buffer.getNumSamples(), midiMessages, false);
}

void MIDIIONode::processBlockBypassed(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
	buffer.clear();
	processMIDIIO(buffer.getNumSamples(), midiMessages, true);
}

void MIDIIONode::processMIDIIO(int numSamples, MidiBuffer& midiMessages, bool bypassed)
{
	if (currentInDevice != nullptr && enabled->boolValue())
	{
		midiCollector.removeNextBlockOfMessages(inMidiBuffer, numSamples);
	}

	if (!inMidiBuffer.isEmpty())
	{
		for (auto& c : outMidiConnections) c->destNode->receiveMIDIFromInput(this, inMidiBuffer);
		if (currentOutDevice != nullptr) currentOutDevice->device->sendBlockOfMessagesNow(inMidiBuffer);
	}
	
	inMidiBuffer.clear();
}