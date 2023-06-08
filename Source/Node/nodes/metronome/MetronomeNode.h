/*
  ==============================================================================

    MetronomeNode.h
    Created: 8 Jun 2023 7:55:44am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

class MetronomeNode :
    public Node,
    public Transport::TransportListener
{
public:
    MetronomeNode(var params = var());
    ~MetronomeNode();

    OwnedArray<AudioFormatReaderSource> ticReaders;
    OwnedArray<AudioTransportSource> ticTransports;

    AudioTransportSource* prevTransport;
    AudioTransportSource* currentTransport;


    void playStateChanged(bool isPlaying, bool forceRestart) override;
    void beatChanged(bool isNewBar, bool isFirstLoopBeat) override;

    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

    DECLARE_TYPE("Metronome");
};