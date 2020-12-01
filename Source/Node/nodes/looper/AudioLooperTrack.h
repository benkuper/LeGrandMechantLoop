/*
  ==============================================================================

    AudioLooperTrack.h
    Created: 1 Dec 2020 11:15:40pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "LooperTrack.h"

class AudioLooperProcessor;

class AudioLooperTrack :
    public LooperTrack
{
public:
    AudioLooperTrack(AudioLooperProcessor* looper, int index, int numChannels);
    ~AudioLooperTrack();

    AudioLooperProcessor* audioLooper;

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