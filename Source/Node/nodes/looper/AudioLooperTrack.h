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
    enum RBEngine { RB_ENGINE_FINER, RB_ENGINE_FASTER };
    enum RBTransients { RB_TRANS_CRISP, RB_TRANS_MIXED, RB_TRANS_SMOOTH };
    enum RBWindow { RB_WIN_STANDARD, RB_WIN_SHORT, RB_WIN_LONG };

    AudioLooperTrack(AudioLooperNode* looper, int index, int numChannels);
    ~AudioLooperTrack();

    AudioLooperNode* audioLooper;
    
    BoolParameter* reverted;
    BoolParameter* rbPreserveFormants;
    BoolParameter* rbSmoothing;
    EnumParameter* rbEngine;
    EnumParameter* rbTransients;
    EnumParameter* rbWindowSize;
    
    int numChannels;
    AudioBuffer<float> buffer;
    AudioBuffer<float> revertedBuffer;
    AudioBuffer<float> rtStretchBuffer;
    AudioBuffer<float> stretchedBuffer;
    AudioBuffer<float> stretchInputBuffer;
    AudioBuffer<float> stretchScratchBuffer;
    AudioBuffer<float> preRecBuffer; //a snapshot of the looper's ringbuffer just before recording. This allows for delay adjustement and nice fades for the end of the loop
    
    bool antiClickFadeBeforeClear;
    bool antiClickFadeBeforeStop;
    bool antiClickFadeBeforePause;
    bool recordOnNextSample;

    int stretchInputSample;
    int stretchPadRemaining;
    int stretchDelayRemaining;

    std::unique_ptr<RubberBand::RubberBandStretcher> stretcher;

    void setNumChannels(int num);
    void updateBufferSize(int newSize);

    void updateStretch(bool force = false) override;
    void updateReverted();
    void resetStretchState(bool clearStretcher = false);
    void rebuildStretcher();
    bool renderStretchedBlock(const AudioBuffer<float>& sourceBuffer, int blockSize);
    void fillCircularBuffer(AudioBuffer<float>& destBuffer, int destStartSample, const AudioBuffer<float>& sourceBuffer, int sourceStartSample, int numSamples) const;

    void stopPlaying() override;
    void clearTrack() override;
    void clearBuffer(bool setIdle = true) override;

    void startRecordingInternal() override;
    void finishRecordingAndPlayInternal() override;
    void retroRecAndPlayInternal() override;

    void processBlock(AudioBuffer<float>& inputBuffer, AudioBuffer<float>& outputBuffer, int numMainChannels, bool outputIfRecording);

    void onContainerParameterChanged(Parameter* p) override;

    virtual void loadSampleFile(File f) override;
    virtual void saveSampleFile(File f) override;

    virtual bool hasContent(bool includeRecordPhase) const override;
};
