/*
  ==============================================================================

    AudioLooperTrack.h
    Created: 1 Dec 2020 11:15:40pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

class AudioLooperNode;

class AudioLooperTrack :
    public LooperTrack
{
public:
    AudioLooperTrack(AudioLooperNode* looper, int index, int numChannels);
    ~AudioLooperTrack();

    AudioLooperNode* audioLooper;

    int numChannels;
    AudioBuffer<float> buffer;
    AudioBuffer<float> rtStretchBuffer;
    AudioBuffer<float> stretchedBuffer;
    AudioBuffer<float> preRecBuffer; //a snapshot of the looper's ringbuffer just before recording. This allows for delay adjustement and nice fades for the end of the loop
    
    bool antiClickFadeBeforeClear;
    bool antiClickFadeBeforeStop;
    bool antiClickFadeBeforePause;
    bool recordOnNextSample;

     std::unique_ptr<RubberBand::RubberBandStretcher> stretcher;

    void setNumChannels(int num);
    void updateBufferSize(int newSize);

    void updateStretch(bool force = false) override;

    void stopPlaying() override;
    void clearTrack() override;
    void clearBuffer(bool setIdle = true) override;

    void startRecordingInternal() override;
    void finishRecordingAndPlayInternal() override;

    void processBlock(AudioBuffer<float>& inputBuffer, AudioBuffer<float>& outputBuffer, int numMainChannels, bool outputIfRecording);

    virtual void loadSampleFile(File f) override;
    virtual void saveSampleFile(File f) override;

    virtual bool hasContent(bool includeRecordPhase) const override;
};