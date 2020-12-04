/*
  ==============================================================================

    MIDILooperNode.cpp
    Created: 1 Dec 2020 11:15:31pm
    Author:  bkupe

  ==============================================================================
*/

#include "MIDILooperNode.h"
#include "MIDILooperTrack.h"
#include "Node/Node.h"
#include "Node/Connection/NodeConnection.h"

MIDILooperProcessor::MIDILooperProcessor(Node* n) :
    LooperProcessor(n, MIDI),
    currentDevice(nullptr)
{
    midiParam = new MIDIDeviceParameter("MIDI Device", true, false);
    ControllableContainer::addParameter(midiParam);

    nodeRef->setMIDIIO(false, true);
}

MIDILooperProcessor::~MIDILooperProcessor()
{

}


void MIDILooperProcessor::initInternal()
{
    LooperProcessor::initInternal();
    updateLooperTracks();
}

void MIDILooperProcessor::setMIDIDevice(MIDIInputDevice* d)
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

LooperTrack* MIDILooperProcessor::createLooperTrack(int index)
{
    return new MIDILooperTrack(this, index);
}

void MIDILooperProcessor::midiMessageReceived(const MidiMessage& m)
{
    collector.addMessageToQueue(m);
    if (m.isNoteOnOrOff())
    {
        for (auto& t : tracksCC.controllableContainers) ((MIDILooperTrack*)t.get())->handleNoteReceived(m);
        if (m.isNoteOn()) currentNoteOns.add({ m.getChannel(), m.getNoteNumber() });
        else if (m.isNoteOff()) currentNoteOns.removeAllInstancesOf({ m.getChannel(), m.getNoteNumber() });
    }
}

void MIDILooperProcessor::onContainerParameterChanged(Parameter* p)
{
    LooperProcessor::onContainerParameterChanged(p);
    if (p == midiParam) setMIDIDevice(midiParam->inputDevice);
}

void MIDILooperProcessor::onControllableFeedbackUpdate(ControllableContainer* cc, Controllable* c)
{
    if (MIDILooperTrack* mt = c->getParentAs<MIDILooperTrack>())
    {
        bool shouldClearNotes = c == mt->active;
        if (c == mt->trackState)
        {
            LooperTrack::TrackState s = mt->trackState->getValueDataAsEnum<LooperTrack::TrackState>();
            if(s == LooperTrack::STOPPED || s == LooperTrack::IDLE)
            {
                shouldClearNotes = true;
            }
        }

        if (shouldClearNotes)
        {
            Array<MIDILooperTrack::SampledNoteInfo> notes = mt->getNoteOnsAtSample(mt->curReadSample);
            for (auto& info : notes) cleanupCollector.addMessageToQueue(MidiMessage(MidiMessage::noteOff(info.channel, info.noteNumber), getBlockSize() - 1));
        }
    }

    LooperProcessor::onControllableFeedbackUpdate(cc, c);
}

void MIDILooperProcessor::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock)
{
    LooperProcessor::prepareToPlay(sampleRate, maximumExpectedSamplesPerBlock);
    collector.reset(sampleRate);
    cleanupCollector.reset(sampleRate);
}

void MIDILooperProcessor::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    MonitorMode mm = monitorMode->getValueDataAsEnum<MonitorMode>();

    int blockSize = buffer.getNumSamples();
    inMidiBuffer.clear();
    collector.removeNextBlockOfMessages(inMidiBuffer, blockSize);

    MidiBuffer outBuffer;
    cleanupCollector.removeNextBlockOfMessages(outBuffer, blockSize);

    for (int i = 0; i < numTracks->intValue(); i++)
    {
        ((MIDILooperTrack*)tracksCC.controllableContainers[i].get())->processBlock(inMidiBuffer, outBuffer, blockSize);
    }

    if (!inMidiBuffer.isEmpty())
    {
        if (mm == ALWAYS || (mm == RECORDING_ONLY && isOneTrackRecording()))
        {
            outBuffer.addEvents(inMidiBuffer, 0, blockSize, 0);
        }
    }
    

    if (!outBuffer.isEmpty())
    {
        for (auto& c : nodeRef->outMidiConnections)
        {
            c->destNode->baseProcessor->receiveMIDIFromInput(nodeRef, outBuffer);
        }
    }
}
