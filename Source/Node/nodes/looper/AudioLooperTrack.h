/*
  ==============================================================================

    AudioLooperTrack.h
    Created: 1 Dec 2020 11:15:40pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "LooperTrack.h"

class AudioLooperNode;

class AudioLooperTrack :
    public LooperTrack
{
public:
    AudioLooperTrack(AudioLooperNode* looper, int index, int numChannels);
    ~AudioLooperTrack();

    AudioLooperNode* audioLooper;

    FloatParameter* volume;
    FloatParameter* rms;

    int numChannels;
    AudioBuffer<float> buffer;
    AudioBuffer<float> preRecBuffer; //a snapshot of the looper's ringbuffer just before recording. This allows for delay adjustement and nice fades for the end of the loop


    void setNumChannels(int num);
    void updateBufferSize(int newSize);
    void clearBuffer() override;

    void startRecordingInternal() override;
    void finishRecordingAndPlayInternal() override;

    void processBlock(AudioBuffer<float>& inputBuffer, AudioBuffer<float>& outputBuffer, int numMainChannels, bool outputIfRecording);
};