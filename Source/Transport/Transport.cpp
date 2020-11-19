/*
  ==============================================================================

	Transport.cpp
	Created: 15 Nov 2020 8:53:43am
	Author:  bkupe

  ==============================================================================
*/

#include "Transport.h"
#include "Engine/AudioManager.h"

juce_ImplementSingleton(Transport)

Transport::Transport() :
	ControllableContainer("Transport"),
	isPlayingParam(nullptr),
	timeInSamples(0),
	curTimeHiRes(0),
	isSettingTempo(false)
{
	bpm = addFloatParameter("BPM", "Current BPM", 120, 10, 900);
	bpm->unitSteps = .2f;

	beatsPerBar = addIntParameter("Beats per Bar", "Number of beats in a bar", 4, 1, 32);
	beatUnit = addIntParameter("Beat unit", "Unit designation of a beat. This is useful to get coherent BPM, visualization, metronome and other cool stuff. This follow musical notation, 4 is a quarter.", 4, 1, 32);

	isPlayingParam = addBoolParameter("Is Playing", "Is it currently playing ?", false);
	isPlayingParam->setControllableFeedbackOnly(true);
	
	currentTime = addFloatParameter("Current Time", "Current time", 0, 0);
	currentTime->setControllableFeedbackOnly(true);

	curBar = addIntParameter("Current Bar", "Current bar", 0, 0);
	curBeat = addIntParameter("Current Beat", "Current beat", 0, 0);
	
	barProgression = addFloatParameter("Bar Progression", "Relative progression of the current bar", 0, 0, 1);
	barProgression->setControllableFeedbackOnly(true);

	beatProgression = addFloatParameter("Beat Progression", "Relative progression of the current beat", 0, 0, 1);
	beatProgression->setControllableFeedbackOnly(true);

	playTrigger = addTrigger("Play", "Start playing");
	togglePlayTrigger = addTrigger("Toggle Play", "Toggle between play / pause");
	pauseTrigger = addTrigger("Pause", "Stops playing but keeps the current time");
	stopTrigger = addTrigger("Stop", "Stops playing and resets current time");

	AudioManager::getInstance()->am.addAudioCallback(this);

}

Transport::~Transport()
{
	AudioManager::getInstance()->am.removeAudioCallback(this);
}

void Transport::clear()
{
	//nothing right now
}

void Transport::play(bool startTempoSet)
{
	isSettingTempo = startTempoSet;
	timeAtStart = Time::getMillisecondCounterHiRes() / 1000.0;
	isPlayingParam->setValue(true);
}

void Transport::pause()
{
	isPlayingParam->setValue(false);
}

void Transport::stop()
{
	pause();
	isPlayingParam->setValue(false);
	setCurrentTime(0);
}

void Transport::finishSetTempo(bool startPlaying)
{
	isSettingTempo = false;
	double t = Time::getMillisecondCounterHiRes() / 1000.0;
	double barLength = t - timeAtStart;
	double beatLength = barLength / beatsPerBar->intValue();
	bpm->setValue(60.0 / beatLength);

	if (startPlaying) playTrigger->trigger();
}

void Transport::setCurrentTimeFromSamples(int samples)
{
	if (timeInSamples == samples) return;
	timeInSamples = samples;
	setCurrentTime(timeInSamples * 1.0 / sampleRate);
}

void Transport::setCurrentTime(double time)
{
	if (curTimeHiRes == time) return;

	curTimeHiRes = time;
	timeInSamples = time* sampleRate;

	currentTime->setValue(curTimeHiRes);
	curBar->setValue(getBarForSamples(timeInSamples));
	curBeat->setValue(getBeatForSamples(timeInSamples));

	float barLength = getBarLength();
	float beatLength = getBeatLength();

	barProgression->setValue(fmod(curTimeHiRes, barLength) / barLength);
	beatProgression->setValue(fmod(curTimeHiRes, beatLength) / beatLength);
}

void Transport::gotoBar(int bar)
{
	setCurrentTime(getTimeForBar(bar));
}

void Transport::gotoBeat(int beat, int bar)
{
	setCurrentTime(getTimeForBeat(beat, bar));
}

void Transport::onContainerTriggerTriggered(Trigger* t)
{
	if (t == playTrigger) play();
	else if (t == togglePlayTrigger)
	{
		if (isPlayingParam->boolValue()) pauseTrigger->trigger();
		else playTrigger->trigger();
	}
	else if (t == pauseTrigger) pause();
	else if (t == stopTrigger) stop();
}

void Transport::onContainerParameterChanged(Parameter* p)
{

}

double Transport::getBarLength() const
{
	return beatsPerBar->intValue() * getBeatLength();
}
double Transport::getBeatLength() const
{
	return 60.0 / bpm->floatValue();
}
double Transport::getBarNumSamples() const
{
	return 	sampleRate * getBarLength();
}
double Transport::getBeatNumSamples() const
{
	return 	sampleRate * getBeatLength();
}
double Transport::getTimeToNextBar() const
{
	return getTimeForBar(curBar->intValue() + 1) - curTimeHiRes;
}

double Transport::getTimeToNextBeat() const
{
	return getTimeForBeat(curBeat->intValue() + 1) - curTimeHiRes;
}

double Transport::getTimeForBar(int bar) const
{
	if (bar < 0) bar = curBar->intValue();
	return bar * getBarLength();
}

double Transport::getTimeForBeat(int beat, int bar, bool relative) const
{
	if (beat < 0) beat = curBeat->intValue();
	return beat * getBeatLength() + (relative ? 0 : getTimeForBar(bar));
}

int Transport::getBarForTime(double time) const
{
	return floor(curTimeHiRes / getBarLength());
}

int Transport::getBeatForTime(double time) const
{
	return floor(curTimeHiRes / getBeatLength());
}

int Transport::getBarForSamples(int samples) const
{
	return floor(samples * 1.0 / (getBeatNumSamples() * beatsPerBar->intValue()));
}

int Transport::getBeatForSamples(int samples, bool relative) const
{
	int numBeats = floor(samples *1.0 / getBeatNumSamples());
	if (relative) numBeats = numBeats % beatsPerBar->intValue();
	return numBeats;
}

double Transport::getBeatTimeInSamples(int beat) const
{
	if (beat < 0) beat = curBeat->intValue();
	return beat * getBeatNumSamples();
}

void Transport::audioDeviceIOCallback(const float** inputChannelData, int numInputChannels, float** outputChannelData, int numOutputChannels, int numSamples)
{
	if (isPlayingParam == nullptr)
	{
		LOG("WEIRD");
		return;
	}

	if (isPlayingParam->boolValue())
	{
		setCurrentTimeFromSamples(timeInSamples + numSamples);
	}
}

void Transport::audioDeviceAboutToStart(AudioIODevice* device)
{
	sampleRate = (int)device->getCurrentSampleRate();
	blockSize = (int)device->getCurrentBufferSizeSamples();

	
}

void Transport::audioDeviceStopped()
{
}

/*
bool Transport::getCurrentPosition(CurrentPositionInfo& result)
{
	result.bpm = bpm->floatValue();
	result.isPlaying = isPlayingParam->boolValue();
	result.isRecording = isSettingTempo;

	result.ppqPosition = (double)(curBeat->intValue()); //??
	result.ppqPositionOfLastBarStart = (double)(getTimeForBar()); // ?? 
	result.ppqLoopStart = 0;
	result.ppqLoopEnd = 0;
	result.timeSigNumerator = beatsPerBar->intValue();
	result.timeSigDenominator = 4;
	result.timeInSamples = timeInSamples;
	result.timeInSeconds = curTimeHiRes;
	result.editOriginTime = 0;
	result.frameRate = FrameRateType::fpsUnknown;
	result.isLooping = false;
	return true;
}
*/