/*
  ==============================================================================

    MIDILooperTrack.h
    Created: 1 Dec 2020 11:16:23pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once


#include "LooperTrack.h"

class MIDILooperNode;

class MIDILooperTrack :
    public LooperTrack
{
public:
    MIDILooperTrack(MIDILooperNode* looper, int index);
    ~MIDILooperTrack();

    MIDILooperNode* midiLooper;

    MidiBuffer buffer;
   
    //for cleaning note Ons
    struct SampledNoteInfo
    {
        int channel;
        int noteNumber;
        int startSample;
        int endSample;
    };

    OwnedArray<SampledNoteInfo> noteInfos;
    Array<int> recNoteIndices; 

    Array<SampledNoteInfo> getNoteOnsAtSample(int sample);

    void clearBuffer() override;
    void startRecordingInternal() override;
    void finishRecordingAndPlayInternal() override;

    void handleNoteReceived(const MidiMessage& m);

    void processBlock(MidiBuffer & inputBuffer, MidiBuffer & outputBuffer, int blockSize);
};