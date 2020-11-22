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
	isCurrentlyPlaying(nullptr),
	sampleRate(0),
	blockSize(0),
	timeInSamples(0),
	numSamplesPerBeat(0),
	isSettingTempo(false),
	setTempoSampleCount(0),
	timeAtStart(0)
{
	bpm = addFloatParameter("BPM", "Current BPM", 120, 10, 900);
	bpm->setControllableFeedbackOnly(true);

	beatsPerBar = addIntParameter("Beats per Bar", "Number of beats in a bar", 4, 1, 32);
	beatUnit = addIntParameter("Beat unit", "Unit designation of a beat. This is useful to get coherent BPM, visualization, metronome and other cool stuff. This follow musical notation, 4 is a quarter.", 4, 1, 32);


	isCurrentlyPlaying = addBoolParameter("Is Playing", "Is it currently playing ?", false);
	isCurrentlyPlaying->setControllableFeedbackOnly(true);
	
	currentTime = addFloatParameter("Current Time", "Current time", 0, 0);
	currentTime->setControllableFeedbackOnly(true);

	curBar = addIntParameter("Current Bar", "Current bar", 0, 0);
	curBeat = addIntParameter("Current Beat", "Current beat", 0, 0);
	curBar->setControllableFeedbackOnly(true);
	curBeat->setControllableFeedbackOnly(true);

	barProgression = addFloatParameter("Bar Progression", "Relative progression of the current bar", 0, 0, 1);
	barProgression->setControllableFeedbackOnly(true);

	beatProgression = addFloatParameter("Beat Progression", "Relative progression of the current beat", 0, 0, 1);
	beatProgression->setControllableFeedbackOnly(true);

	playTrigger = addTrigger("Play", "Start playing");
	togglePlayTrigger = addTrigger("Toggle Play", "Toggle between play / pause");
	pauseTrigger = addTrigger("Pause", "Stops playing but keeps the current time");
	stopTrigger = addTrigger("Stop", "Stops playing and resets current time");

	quantization = addEnumParameter("Quantization", "Default quantization for nodes.");
	quantization->addOption("Bar", BAR)->addOption("Beat", BEAT)->addOption("Free", FREE);

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

	if (!startTempoSet) isCurrentlyPlaying->setValue(true);
	else setTempoSampleCount = 0;
}

void Transport::pause()
{
	isCurrentlyPlaying->setValue(false);
}

void Transport::stop()
{
	pause();
	isSettingTempo = false;
	isCurrentlyPlaying->setValue(false);
	setCurrentTime(0);
}

void Transport::finishSetTempo(bool startPlaying)
{
	isSettingTempo = false;
	numSamplesPerBeat = round(setTempoSampleCount / (beatsPerBar->intValue() * blockSize)) * blockSize;
	bpm->setValue(60.0 / getTimeForSamples(numSamplesPerBeat));
	timeInSamples = 0;

	LOG("Finish set tempo, sample count : " << setTempoSampleCount);

	curBar->setValue(0);
	curBeat->setValue(0);

	if (startPlaying) playTrigger->trigger();
}

void Transport::setCurrentTime(int samples)
{
	if (timeInSamples == samples) return;
	timeInSamples = samples;
	barProgression->setValue(getRelativeBarSamples() * 1.0 / getBarNumSamples());
	beatProgression->setValue(getRelativeBeatSamples() * 1.0 / getBeatNumSamples());
	
	int prevBar = curBar->intValue();
	int prevBeat = curBeat->intValue();

	curBar->setValue(getBarForSamples(timeInSamples));
	curBeat->setValue(getBeatForSamples(timeInSamples));

	bool barChanged = prevBar != curBar->intValue();
	bool beatChanged = prevBeat != curBeat->intValue();
	if(barChanged || beatChanged) transportListeners.call(&TransportListener::beatChanged, barChanged);
}

void Transport::gotoBar(int bar)
{
	setCurrentTime(getSamplesForBar(bar));
}

void Transport::gotoBeat(int beat, int bar)
{
	setCurrentTime(getSamplesForBeat(beat, bar, false));
}

void Transport::onContainerTriggerTriggered(Trigger* t)
{
	if (t == playTrigger) play();
	else if (t == togglePlayTrigger)
	{
		if (isCurrentlyPlaying->boolValue()) pauseTrigger->trigger();
		else playTrigger->trigger();
	}
	else if (t == pauseTrigger) pause();
	else if (t == stopTrigger) stop();
}

void Transport::onContainerParameterChanged(Parameter* p)
{
	if (p == isCurrentlyPlaying)
	{
		transportListeners.call(&TransportListener::playStateChanged, isCurrentlyPlaying->boolValue());
	}
}

int Transport::getBarNumSamples() const
{
	return 	numSamplesPerBeat * beatsPerBar->intValue();
}

int Transport::getBeatNumSamples() const
{
	return 	numSamplesPerBeat;
}

int Transport::getSamplesToNextBar() const
{
	return getBarNumSamples() - getRelativeBarSamples();
}

int Transport::getSamplesToNextBeat() const
{
	return numSamplesPerBeat - getRelativeBeatSamples();
}

int Transport::getRelativeBarSamples() const
{
	return timeInSamples% getBarNumSamples();
}

int Transport::getRelativeBeatSamples() const
{
	return timeInSamples % numSamplesPerBeat;
}

int Transport::getBarForSamples(int samples, bool floorResult) const
{
	double numBarsD = samples * 1.0 / getBarNumSamples();
	return floorResult ? floor(numBarsD) : round(numBarsD);
}

int Transport::getBeatForSamples(int samples, bool relative, bool floorResult) const
{
	double numBeatsD = floor(samples * 1.0 / getBeatNumSamples());
	int numBeats = floorResult ? floor(numBeatsD) : round(numBeatsD);
	if (relative) numBeats = numBeats % beatsPerBar->intValue();
	return numBeats;
}

int Transport::getSamplesForBar(int bar) const
{
	if (bar < 0) bar = curBar->intValue();
	return bar * getBarNumSamples();
}

int Transport::getSamplesForBeat(int beat, int bar, bool relative) const
{
	if (beat < 0) beat = curBeat->intValue();
	return beat * getBeatNumSamples() + (relative ? 0 : getSamplesForBar(bar));
}

double Transport::getBarLength() const
{
	return getTimeForSamples(getBarNumSamples());
}
double Transport::getBeatLength() const
{
	return getTimeForSamples(numSamplesPerBeat);
}

double Transport::getTimeToNextBar() const
{
	return getTimeForBar(curBar->intValue() + 1);
}

double Transport::getTimeToNextBeat() const
{
	return getTimeForBeat(curBeat->intValue() + 1);
}

double Transport::getTimeForSamples(int samples) const
{
	return samples * 1.0 / sampleRate;
}

double Transport::getCurrentTime() const
{
	return getTimeForSamples(timeInSamples);
}

double Transport::getTimeForBar(int bar) const
{
	return getTimeForSamples(getSamplesForBar(bar));
}

double Transport::getTimeForBeat(int beat, int bar, bool relative) const
{
	return getTimeForSamples(getSamplesForBeat(beat, bar, relative));
}

int Transport::getBarForTime(double time, bool floorResult) const
{
	int timeInSamples = getSamplesForTime(time);
	double numBarsD = floor(timeInSamples *1.0 / getBarNumSamples());
	return floorResult ? floor(numBarsD) : round(numBarsD);

}

int Transport::getBeatForTime(double time, bool relative, bool floorResult) const
{
	int timeInSamples = getSamplesForTime(time);
	double numBeatsD = timeInSamples * 1.0 / getBeatNumSamples();
	int numBeats = floorResult ? floor(numBeatsD) : round(numBeatsD);
	if (relative) numBeats = numBeats % beatsPerBar->intValue();
	return numBeats;
}

int Transport::getSamplesForTime(double time, bool blockPerfect) const
{
	int numSamples =  time * sampleRate;
	if (blockPerfect) numSamples = (numSamples / getBarNumSamples()) * getBarNumSamples();
	return numSamples;
}

int Transport::getTotalBeatCount() const
{
	return curBar->intValue() * beatsPerBar->intValue() + curBeat->intValue();
}

void Transport::audioDeviceIOCallback(const float** inputChannelData, int numInputChannels, float** outputChannelData, int numOutputChannels, int numSamples)
{
	if (isCurrentlyPlaying->boolValue())
	{
		setCurrentTime(timeInSamples + numSamples);
	}
	else if (isSettingTempo)
	{
		setTempoSampleCount += numSamples;
	}
}

void Transport::audioDeviceAboutToStart(AudioIODevice* device)
{
	sampleRate = (int)device->getCurrentSampleRate();
	blockSize = (int)device->getCurrentBufferSizeSamples();
	if (numSamplesPerBeat == 0) numSamplesPerBeat = sampleRate * 60.0 / bpm->floatValue();
	
}

void Transport::audioDeviceStopped()
{
}

/*
bool Transport::getCurrentPosition(CurrentPositionInfo& result)
{
	result.bpm = bpm->floatValue();
	result.isPlaying = isCurrentlyPlaying->boolValue();
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