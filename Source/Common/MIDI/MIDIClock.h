/*
 ==============================================================================

 Copyright © Organic Orchestra, 2017

 This file is part of LGML. LGML is a software to manipulate sound in real-time

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation (version 3 of the License).

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

 ==============================================================================
 */

#pragma once

#include "Transport/Transport.h"
#include "MIDIManager.h"

constexpr int MIDI_SYNC_QUEUE_SIZE = 100;

class MIDIClock :
    public Transport::TransportListener
{
public:
    MIDIClock(bool sendSPP);
    ~MIDIClock();

    bool start();
    void stop();
    bool setOutput(MIDIOutputDevice * _midiOut);
    void reset();

    bool sendSPP;
    float delta;
    
    bool isRunning();
private:
    static double sendAllClocks();
    double runClock();

    MidiMessage nextMidiMsg;
    MIDIOutputDevice * midiOut;

    bool hasSentMessage = false;
    
    friend class MIDIClockRunner;
    Atomic<bool> _isRunning = false;
    // TimeManager Listener

     void bpmChanged () override;
     virtual void beatChanged(bool isNewBar); //this allow for event after both bar and beat have been updated
     virtual void playStateChanged(bool isPlaying, bool forceRestart);

    void addClockIfNeeded(double & timeToNextMessage);
    int appendCurrentSPP();
    
    double getPPQWithDelta(int multiplier);
    static double ppqToTime(double ppq,int multiplier);
    void appendOneMsg(const MidiMessage & msg);
    void appendClocks(const int num);
    MidiMessage  messagesToSend[MIDI_SYNC_QUEUE_SIZE];
    AbstractFifo midiFifo;

    private :
    struct MIDIClockState{
        MIDIClockState():isPlaying(false),ppqn(0){}
        bool isPlaying;
        Atomic<int> ppqn; // has to be an integer type (same resolution as device)


    };
    MIDIClockState state;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MIDIClock)
    
};



