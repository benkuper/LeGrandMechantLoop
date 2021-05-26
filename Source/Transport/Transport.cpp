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
	bpm->defaultUI = FloatParameter::LABEL;
	//bpm->setControllableFeedbackOnly(true);

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

	firstLoopBeats = addIntParameter("First Loop Beat", "Number of beats that the loop that set the tempo is taking", 1, 1);
	firstLoopBeats->setControllableFeedbackOnly(true);

	playTrigger = addTrigger("Play", "Start playing");
	togglePlayTrigger = addTrigger("Toggle Play", "Toggle between play / stop");
	pauseTrigger = addTrigger("Pause", "Stops playing but keeps the current time");
	stopTrigger = addTrigger("Stop", "Stops playing and resets current time");

	quantization = addEnumParameter("Quantization", "Default quantization for nodes.");
	quantization->addOption("First Loop", FIRSTLOOP)->addOption("Bar", BAR)->addOption("Beat", BEAT)->addOption("Free", FREE);

	recQuantization = addEnumParameter("Rec Quantization Mode", "Quantization mode when setting tempo on rec");
	recQuantization->addOption("Auto", REC_AUTO)->addOption("Bar", REC_BAR)->addOption("Beat", REC_BEAT);

	recQuantizBPMRange = addPoint2DParameter("Auto Quantiz BPM Range","Range to auto setting tempo on rec");
	recQuantizBPMRange->setBounds(20, 20, 500, 500);
	recQuantizBPMRange->setPoint(50, 190);
	recQuantizCount = addIntParameter("Manual Quantiz Count", "", 1, 1, 100, false);

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

void Transport::play(bool startTempoSet, bool playFromStart)
{
	isSettingTempo = startTempoSet;
	
	if (isCurrentlyPlaying->boolValue() && !playFromStart) return;
	
	if (playFromStart)
	{
		setCurrentTime(0);
		timeAtStart = Time::getMillisecondCounterHiRes() / 1000.0;
	}

	if (!startTempoSet)
	{
		int rawSamplesPerBeat = round(sampleRate * 60.0 / bpm->floatValue());
		numSamplesPerBeat = rawSamplesPerBeat - rawSamplesPerBeat % blockSize;
		timeInSamples = 0;

		isCurrentlyPlaying->setValue(true);
		transportListeners.call(&TransportListener::playStateChanged, isCurrentlyPlaying->boolValue(), playFromStart);
	}
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
	if (blockSize == 0) return;
	
	isSettingTempo = false;
	
	RecQuantization rq = recQuantization->getValueDataAsEnum<RecQuantization>();
	int targetNumBeats = 0;
	if (rq == REC_AUTO)
	{
		targetNumBeats = beatsPerBar->intValue();
		int targetRawSamplesPerBeat = floor(setTempoSampleCount / targetNumBeats);
		float targetSamplesPerBeat = targetRawSamplesPerBeat - targetRawSamplesPerBeat % blockSize;
		float expectedBPM = 60.0 / getTimeForSamples(targetSamplesPerBeat);

		if (expectedBPM < recQuantizBPMRange->x)
		{
			while (expectedBPM < recQuantizBPMRange->x)
			{
				targetNumBeats *= 2;
				expectedBPM *= 2;
			}
		}
		else if (expectedBPM > recQuantizBPMRange->y)
		{
			targetNumBeats /= beatsPerBar->intValue();
			expectedBPM /= beatsPerBar->intValue();

			if (expectedBPM < recQuantizBPMRange->x)
			{
				targetNumBeats *= 2;
				expectedBPM *= 2;
			}
		}
	}
	else if (rq == REC_BAR || rq == REC_BEAT)
	{
		targetNumBeats = recQuantizCount->intValue() * (rq == REC_BAR ? beatsPerBar->intValue() : 1);
	}
	
	int rawSamplesPerBeat = floor(setTempoSampleCount / targetNumBeats);
	numSamplesPerBeat = rawSamplesPerBeat - rawSamplesPerBeat % blockSize;
	float targetBPM = 60.0 / getTimeForSamples(numSamplesPerBeat);
	bpm->setValue(targetBPM);
	timeInSamples = 0;
	
	jassert(numSamplesPerBeat % blockSize == 0);

	firstLoopBeats->setValue(targetNumBeats);
	curBar->setValue(0);
	curBeat->setValue(0);


	transportListeners.call(&TransportListener::beatNumSamplesChanged);
	
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
	if (barChanged || beatChanged)
	{
		bool firstLoop = getTotalBeatCount() % firstLoopBeats->intValue() == 0;
		transportListeners.call(&TransportListener::beatChanged, barChanged, firstLoop);
	}
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
	if (t == playTrigger) play(false, true);
	else if (t == togglePlayTrigger)
	{
		if (isCurrentlyPlaying->boolValue()) stopTrigger->trigger();
		else playTrigger->trigger();
	}
	else if (t == pauseTrigger) pause();
	else if (t == stopTrigger) stop();
}

void Transport::onContainerParameterChanged(Parameter* p)
{
	if (p == isCurrentlyPlaying)
	{
		transportListeners.call(&TransportListener::playStateChanged, isCurrentlyPlaying->boolValue(), false);
	}
	else if (p == recQuantization)
	{
		RecQuantization rq = recQuantization->getValueDataAsEnum<RecQuantization>();
		recQuantizBPMRange->setEnabled(rq == REC_AUTO);
		recQuantizCount->setEnabled(rq != REC_AUTO);
	}
	else if (p == bpm)
	{
		transportListeners.call(&TransportListener::bpmChanged);
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

int Transport::getBlockPerfectNumSamples(int samples, bool floorResult) const
{
	double numBlocksD = samples * 1.0 / blockSize;
	int numBlocks = floorResult?floor(numBlocksD):round(numBlocksD);
	return numBlocks * blockSize;
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

double Transport::getCurrentTimeInBeats() const
{
	return getCurrentTime() / getTimeForBeat(1);
}

double Transport::getCurrentTimeInBars() const
{
	return getCurrentTime() / getTimeForBar(1);
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

	for (int i = 0; i < numOutputChannels; i++) FloatVectorOperations::clear(outputChannelData[i], numSamples);
}

void Transport::audioDeviceAboutToStart(AudioIODevice* device)
{
	sampleRate = (int)device->getCurrentSampleRate();
	blockSize = (int)device->getCurrentBufferSizeSamples();

	int rawSamplesPerBeat = round(sampleRate * 60.0 / bpm->floatValue());
	numSamplesPerBeat = rawSamplesPerBeat - rawSamplesPerBeat % blockSize;
	timeInSamples = 0;
}

void Transport::audioDeviceStopped()
{
}

bool Transport::getCurrentPosition(CurrentPositionInfo& result)
{
	result.bpm = bpm->floatValue();
	result.isPlaying = isCurrentlyPlaying->boolValue();
	result.isRecording = isSettingTempo;

	result.ppqPosition = (double)(curBeat->intValue()); //??
	result.ppqPositionOfLastBarStart = (double)(curBar->intValue() * beatsPerBar->intValue()); // ?? 
	result.ppqLoopStart = 0;
	result.ppqLoopEnd = 0;
	result.timeSigNumerator = beatsPerBar->intValue();
	result.timeSigDenominator = 4;
	result.timeInSamples = timeInSamples;
	result.timeInSeconds = getCurrentTime();
	result.editOriginTime = 0;
	result.frameRate = FrameRateType::fpsUnknown;
	result.isLooping = false;
	return true;
}