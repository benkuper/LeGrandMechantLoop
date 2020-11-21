/*
  ==============================================================================

    LooperTrack.cpp
    Created: 21 Nov 2020 6:33:06pm
    Author:  bkupe

  ==============================================================================
*/

#include "LooperTrack.h"
#include "Transport/Transport.h"

String LooperTrack::trackStateNames[LooperTrack::STATES_MAX] = { "Idle", "Will Record", "Recording", "Finish Recording", "Playing", "Will Stop", "Stopped", "Will Play"};

LooperTrack::LooperTrack(int index, int numChannels) :
    ControllableContainer("Track "+String(index+1)),
    index(index),
    numChannels(numChannels),
    curSample(0)
{
    editorIsCollapsed = true;

    trackState = addEnumParameter("State", "State of this track, for feedback");
    for (int i = 0; i < STATES_MAX; i++) trackState->addOption(trackStateNames[i], (TrackState)i);
    trackState->setControllableFeedbackOnly(true);

    playRecordTrigger = addTrigger("Play & Record", "If empty, this will start recording. If recording, this will stop recording and start playing. If already recorded, this will start playing");
    stopTrigger = addTrigger("Stop", "If recording, this will cancel the recording. If playing, this will stop the track but will keep the recorded content");
    clearTrigger = addTrigger("Clear", "If recording, this will cancel the recording. If playing, this will clear the track of the recorded content");

    active = addBoolParameter("Active", "If this is not checked, this will act as a track mute.", true);
    volume = addFloatParameter("Gain", "The gain for this track", 1, 0, 2);
    rms = addFloatParameter("RMS", "RMS for this track, for feedback", 0, 0, 1);
    rms->setControllableFeedbackOnly(true);

    updateAudioBuffer();
}

LooperTrack::~LooperTrack()
{
}

void LooperTrack::setNumChannels(int num)
{
    if (num == numChannels) return;
    numChannels = num;
    updateAudioBuffer();
}

void LooperTrack::updateAudioBuffer()
{
    buffer.setSize(numChannels, Transport::getInstance()->getBarNumSamples() * 10, true);
}

void LooperTrack::recordOrPlayNow()
{
    TrackState s = trackState->getValueDataAsEnum<TrackState>();

    switch (s)
    {
    case WILL_RECORD:
        startRecording();
        break;

    case WILL_PLAY:
        stopRecordAndPlay();
        break;

    default:
        break;
    }
}

void LooperTrack::stateChanged()
{
    TrackState s = trackState->getValueDataAsEnum<TrackState>();

    switch (s)
    {
    case IDLE:
        break;
    case WILL_RECORD:
        break;
    case RECORDING:
        break;
    case WILL_PLAY:
        break;
    case PLAYING:
        break;
    }
}

void LooperTrack::startRecording()
{
    trackState->setValueWithData(RECORDING);
}

void LooperTrack::stopRecordAndPlay()
{
    trackState->setValueWithData(PLAYING);
}

void LooperTrack::clearContent()
{
    buffer.clear();
    curSample = 0;
}

void LooperTrack::onContainerTriggerTriggered(Trigger* t)
{
    if (t == playRecordTrigger)
    {

    }
    else if (t == stopTrigger)
    {

    }
    else if (t == clearTrigger)
    {

    }
}

void LooperTrack::onContainerParameterChanged(Parameter* p)
{
    if (p == trackState)
    {
        stateChanged();
    } 
}

void LooperTrack::handleNewBeat()
{
    Transport::Quantization q = Transport::getInstance()->quantization->getValueDataAsEnum<Transport::Quantization>();
    if (q != Transport::BEAT) return;
    
    recordOrPlayNow();
}

void LooperTrack::handleNewBar()
{
    Transport::Quantization q = Transport::getInstance()->quantization->getValueDataAsEnum<Transport::Quantization>();
    if (q != Transport::BAR) return;
    
    recordOrPlayNow();

}

void LooperTrack::processBlock(AudioBuffer<float>& inputBuffer)
{
    if (!isRecording())
    {
        jassert(numChannels <= inputBuffer.getNumChannels());

        for (int i = 0; i < numChannels; i++)
        {
            buffer.copyFrom(i,curSample, inputBuffer, i, 0, inputBuffer.getNumSamples());
        }
    }
}

bool LooperTrack::isRecording() const
{
    TrackState s = trackState->getValueDataAsEnum<TrackState>();
    return s ==  RECORDING || s == FINISH_RECORDING;
}

bool LooperTrack::isPlaying() const
{
    TrackState s = trackState->getValueDataAsEnum<TrackState>();
    return s == PLAYING || s == WILL_STOP;
}