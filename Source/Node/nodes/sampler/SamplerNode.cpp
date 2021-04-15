/*
  ==============================================================================

    SamplerNode.cpp
    Created: 14 Apr 2021 8:40:43am
    Author:  bkupe

  ==============================================================================
*/

#include "SamplerNode.h"
#include "ui/SamplerNodeUI.h"
#include "../../Connection/NodeConnection.h"

SamplerNode::SamplerNode(var params) :
    Node(getTypeStringStatic(), params, true, true, true, true),
	currentDevice(nullptr)
{
	midiParam = new MIDIDeviceParameter("MIDI Device", true, false);
	ControllableContainer::addParameter(midiParam);

	attack = addFloatParameter("Attack", "Time of the attack in seconds", 0, 0);
	attack->defaultUI = FloatParameter::TIME;
	decay = addFloatParameter("Decay", "Time of decay in seconds", 0, 0);
	decay->defaultUI = FloatParameter::TIME;
	sustain = new DecibelFloatParameter("Sustain", "Sustain dB");
	release = addFloatParameter("Release", "Time of the release in seconds", 0, 0);
	release->defaultUI = FloatParameter::TIME;

	setMIDIIO(true, true);
}

SamplerNode::~SamplerNode()
{
	setMIDIDevice(nullptr);
}

void SamplerNode::exportSampleSet(File folder)
{
}

void SamplerNode::importSampleSet(File folder)
{
}

void SamplerNode::setMIDIDevice(MIDIInputDevice* d)
{
	if (currentDevice == d) return;
	if (currentDevice != nullptr)
	{
		currentDevice->removeMIDIInputListener(this);
	}

	currentDevice = d;

	if (currentDevice != nullptr)
	{
		currentDevice->addMIDIInputListener(this);
	}
}

void SamplerNode::onContainerParameterChangedInternal(Parameter* p)
{
	Node::onContainerParameterChangedInternal(p);
	if (p == midiParam) setMIDIDevice(midiParam->inputDevice);
}

void SamplerNode::controllableStateChanged(Controllable* c)
{
	Node::controllableStateChanged(c);
}

void SamplerNode::midiMessageReceived(const MidiMessage& m)
{
	midiCollector.addMessageToQueue(m); //ugly hack to have at least events sorted, but sampleNumber should be exact
}

void SamplerNode::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock)
{
	if (sampleRate != 0) midiCollector.reset(sampleRate);
}

void SamplerNode::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
	if (currentDevice != nullptr)
	{
		midiCollector.removeNextBlockOfMessages(inMidiBuffer, buffer.getNumSamples());
	}

	//
	keyboardState.processNextMidiBuffer(inMidiBuffer, 0, buffer.getNumSamples(), false);

	if (!inMidiBuffer.isEmpty())
	{
		for (auto& c : outMidiConnections) c->destNode->receiveMIDIFromInput(this, inMidiBuffer);
	}
	
	inMidiBuffer.clear();
}


BaseNodeViewUI* SamplerNode::createViewUI()
{
    return new SamplerNodeViewUI(this);
}
