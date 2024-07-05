/*
  ==============================================================================

    RecorderNode.h
    Created: 27 May 2021 1:28:22pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

class RecorderNode :
    public Node,
    public Transport::TransportListener
{
public:
    RecorderNode(var params);
    ~RecorderNode();

    FileParameter* recFolder;
    StringParameter* baseName;
    
    
    BoolParameter* autoRecOnPlay;
    BoolParameter* autoStopOnStop;
    FloatParameter* stopVolumeThreshold;
    FloatParameter* stopVolumeTime;
    BoolParameter* recSeparateFiles;
    
    bool stopOnNextSilence;
    double timeSinceLastVolumeUnderThreshold;

    Trigger* recTrigger;
    BoolParameter* isRecording;

    //Recording
    TimeSliceThread backgroundThread{ "Audio Recorder Thread" }; // the thread that will write our audio data to disk
    
    OwnedArray<AudioFormatWriter::ThreadedWriter> threadedWriters; // the FIFO used to buffer the incoming data
    double sampleRate;
    int64 nextSampleNum = 0;

    CriticalSection writerLock;
    OwnedArray<std::atomic<AudioFormatWriter::ThreadedWriter*>> activeWriters;


    void startRecording();
    void stopRecording();

    void onContainerParameterChangedInternal(Parameter* p) override;
    void onContainerTriggerTriggered(Trigger* t) override;

    void playStateChanged(bool isPlaying, bool forceRestart) override;

    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

    String getTypeString() const override { return getTypeStringStatic(); }
    static const String getTypeStringStatic() { return "Audio Recorder"; }
};