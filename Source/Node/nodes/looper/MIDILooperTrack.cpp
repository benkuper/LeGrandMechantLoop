/*
  ==============================================================================

    MIDILooperTrack.cpp
    Created: 1 Dec 2020 11:16:23pm
    Author:  bkupe

  ==============================================================================
*/

#include "MIDILooperTrack.h"
#include "MIDILooperNode.h"

MIDILooperTrack::MIDILooperTrack(MIDILooperProcessor* looper, int index) :
    LooperTrack(looper, index),
	midiLooper(looper)
{
	
}

MIDILooperTrack::~MIDILooperTrack()
{
}

void MIDILooperTrack::clearBuffer()
{
    buffer.clear();
	LooperTrack::clearBuffer();
}

void MIDILooperTrack::startRecordingInternal()
{
}

void MIDILooperTrack::finishRecordingAndPlayInternal()
{
	for (auto& i : recNoteIndices) //add current noteOns from looper to noteToClean array
	{
		MidiMessage m = MidiMessage::noteOff(noteInfos[i]->channel, noteInfos[i]->noteNumber);
		buffer.addEvent(m, curSample - 1);
	}

	recNoteIndices.clear();
}

void MIDILooperTrack::handleNoteReceived(const MidiMessage& m)
{
	if (isRecording(false))
	{
		if (m.isNoteOn())
		{
			SampledNoteInfo * info = new SampledNoteInfo({m.getChannel(), m.getNoteNumber(), curSample, -1 });
			noteInfos.add(info);
			recNoteIndices.add(noteInfos.size()-1);
		}
		else
		{
			for (auto& i : recNoteIndices)
			{
				SampledNoteInfo* info = noteInfos[i];
				if (info->channel == m.getChannel() && info->noteNumber == m.getNoteNumber())
				{
					info->endSample = curSample;
					recNoteIndices.removeAllInstancesOf(i);
					break;
				}
			}
		}
	}
}

Array<MIDILooperTrack::SampledNoteInfo> MIDILooperTrack::getNoteOnsAtSample(int sample)
{
	Array<SampledNoteInfo> result;
	for (auto& n : noteInfos)
	{
		if (n->startSample <= sample && (n->endSample >= sample || n->endSample == -1)) result.add({ n->channel, n->noteNumber });
	}
	return result;
}


void MIDILooperTrack::processBlock(MidiBuffer& inputBuffer, MidiBuffer& outputBuffer, int blockSize)
{
	if (isRecording(false))
	{
		if (!finishRecordLock)
		{
			buffer.addEvents(inputBuffer, 0, blockSize, curSample);
		}
	}
	
	LooperTrack::processTrack(blockSize);

	if (active->boolValue() && isPlaying(false))
	{
		outputBuffer.addEvents(buffer, curReadSample, blockSize, -curReadSample);
	}
}
