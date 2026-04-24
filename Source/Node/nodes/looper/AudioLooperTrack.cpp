/*
  ==============================================================================

	AudioLooperTrack.cpp
	Created: 1 Dec 2020 11:15:40pm
	Author:  bkupe

  ==============================================================================
*/

#include "Node/NodeIncludes.h"

AudioLooperTrack::AudioLooperTrack(AudioLooperNode* looper, int index, int numChannels) :
	LooperTrack(looper, index),
	audioLooper(looper),
	numChannels(numChannels),
	rtStretchBuffer(numChannels, Transport::getInstance()->blockSize),
	stretchInputBuffer(numChannels, Transport::getInstance()->blockSize),
	stretchScratchBuffer(numChannels, Transport::getInstance()->blockSize),
	antiClickFadeBeforeClear(true), //needs that otherwise first clear doesn't work
	antiClickFadeBeforeStop(false),
	antiClickFadeBeforePause(false),
	stretchInputSample(0),
	stretchPadRemaining(0),
	stretchDelayRemaining(0)
{
	reverted = addBoolParameter("Reverted", "Play this track backwards.", false);
	rbPreserveFormants = addBoolParameter("RB Formants", "Preserve formants while time-stretching.", true);
	rbSmoothing = addBoolParameter("RB Smoothing", "Enable Rubber Band smoothing for live tempo changes.", true);

	rbEngine = addEnumParameter("RB Engine", "Rubber Band realtime engine.");
	rbEngine->addOption("Finer", RB_ENGINE_FINER)->addOption("Faster", RB_ENGINE_FASTER);

	rbTransients = addEnumParameter("RB Transients", "Transient handling for the faster realtime engine.");
	rbTransients->addOption("Crisp", RB_TRANS_CRISP)->addOption("Mixed", RB_TRANS_MIXED)->addOption("Smooth", RB_TRANS_SMOOTH);

	rbWindowSize = addEnumParameter("RB Window", "Rubber Band window size.");
	rbWindowSize->addOption("Short", RB_WIN_SHORT)->addOption("Standard", RB_WIN_STANDARD)->addOption("Long", RB_WIN_LONG);
}

AudioLooperTrack::~AudioLooperTrack()
{
}


void AudioLooperTrack::setNumChannels(int num)
{
	if (num == numChannels) return;
	numChannels = num;
	updateBufferSize(buffer.getNumSamples());
	clearBuffer();
}

void AudioLooperTrack::updateBufferSize(int newSize)
{
	buffer.setSize(numChannels, newSize, true, true);
}

void AudioLooperTrack::resetStretchState(bool clearStretcher)
{
	stretchInputSample = 0;
	stretchPadRemaining = 0;
	stretchDelayRemaining = 0;
	rtStretchBuffer.clear();
	stretchedBuffer.clear();
	stretchInputBuffer.clear();
	stretchScratchBuffer.clear();

	if (clearStretcher)
	{
		stretcher.reset();
	}
	else if (stretcher != nullptr)
	{
		stretcher->reset();
	}
}

void AudioLooperTrack::rebuildStretcher()
{
	if (stretch == 1 || numChannels <= 0 || bufferNumSamples <= 0)
	{
		resetStretchState(true);
		return;
	}

	int rbOptions = RubberBand::RubberBandStretcher::OptionProcessRealTime
		| RubberBand::RubberBandStretcher::OptionChannelsTogether;

	if (rbEngine->getValueDataAsEnum<RBEngine>() == RB_ENGINE_FINER) rbOptions |= RubberBand::RubberBandStretcher::OptionEngineFiner;
	if (rbPreserveFormants->boolValue()) rbOptions |= RubberBand::RubberBandStretcher::OptionFormantPreserved;
	if (rbSmoothing->boolValue()) rbOptions |= RubberBand::RubberBandStretcher::OptionSmoothingOn;

	switch (rbTransients->getValueDataAsEnum<RBTransients>())
	{
	case RB_TRANS_MIXED: rbOptions |= RubberBand::RubberBandStretcher::OptionTransientsMixed; break;
	case RB_TRANS_SMOOTH: rbOptions |= RubberBand::RubberBandStretcher::OptionTransientsSmooth; break;
	default: break;
	}

	switch (rbWindowSize->getValueDataAsEnum<RBWindow>())
	{
	case RB_WIN_SHORT: rbOptions |= RubberBand::RubberBandStretcher::OptionWindowShort; break;
	case RB_WIN_LONG: rbOptions |= RubberBand::RubberBandStretcher::OptionWindowLong; break;
	default: break;
	}

	stretcher.reset(new RubberBand::RubberBandStretcher(looper->processor->getSampleRate(), numChannels, rbOptions));

	const int maxProcessSize = jmax(looper->processor->getBlockSize(), 1);
	stretcher->setMaxProcessSize((size_t)maxProcessSize);
	stretcher->setTimeRatio(stretch);

	rtStretchBuffer.setSize(numChannels, maxProcessSize, false, false, true);
	stretchInputBuffer.setSize(numChannels, maxProcessSize, false, false, true);
	stretchScratchBuffer.setSize(numChannels, maxProcessSize, false, false, true);

	stretchInputSample = 0;
	stretchPadRemaining = (int)stretcher->getPreferredStartPad();
	stretchDelayRemaining = (int)stretcher->getStartDelay();
}

void AudioLooperTrack::fillCircularBuffer(AudioBuffer<float>& destBuffer, int destStartSample, const AudioBuffer<float>& sourceBuffer, int sourceStartSample, int numSamples) const
{
	const int sourceNumSamples = sourceBuffer.getNumSamples();
	if (numSamples <= 0 || sourceNumSamples <= 0) return;

	int writeOffset = destStartSample;
	int readOffset = sourceStartSample % sourceNumSamples;
	if (readOffset < 0) readOffset += sourceNumSamples;
	int remaining = numSamples;

	while (remaining > 0)
	{
		const int chunkSize = jmin(remaining, sourceNumSamples - readOffset);

		for (int channel = 0; channel < numChannels; ++channel)
		{
			destBuffer.copyFrom(channel, writeOffset, sourceBuffer, channel, readOffset, chunkSize);
		}

		writeOffset += chunkSize;
		remaining -= chunkSize;
		readOffset = 0;
	}
}

bool AudioLooperTrack::renderStretchedBlock(const AudioBuffer<float>& sourceBuffer, int blockSize)
{
	if (stretcher == nullptr || sourceBuffer.getNumSamples() == 0 || blockSize <= 0)
	{
		return false;
	}

	rtStretchBuffer.setSize(numChannels, blockSize, false, false, true);
	rtStretchBuffer.clear();

	if (stretchInputBuffer.getNumSamples() < blockSize) stretchInputBuffer.setSize(numChannels, blockSize, false, false, true);
	if (stretchScratchBuffer.getNumSamples() < blockSize) stretchScratchBuffer.setSize(numChannels, blockSize, false, false, true);

	const int sourceNumSamples = sourceBuffer.getNumSamples();
	int renderedSamples = 0;
	int guard = 0;

	while (renderedSamples < blockSize && guard++ < 64)
	{
		while (stretchDelayRemaining > 0 && stretcher->available() > 0)
		{
			const int chunkSize = jmin<int>(jmin<int>((int)stretcher->available(), stretchDelayRemaining), stretchScratchBuffer.getNumSamples());
			if (chunkSize <= 0) break;

			HeapBlock<float*> discardPointers(numChannels);
			for (int channel = 0; channel < numChannels; ++channel) discardPointers[channel] = stretchScratchBuffer.getWritePointer(channel);

			const int retrieved = (int)stretcher->retrieve(discardPointers.get(), (size_t)chunkSize);
			if (retrieved <= 0) break;

			stretchDelayRemaining -= retrieved;
		}

		if (stretchDelayRemaining == 0 && stretcher->available() > 0)
		{
			const int chunkSize = jmin<int>((int)stretcher->available(), blockSize - renderedSamples);
			if (chunkSize > 0)
			{
				HeapBlock<float*> renderPointers(numChannels);
				for (int channel = 0; channel < numChannels; ++channel) renderPointers[channel] = rtStretchBuffer.getWritePointer(channel, renderedSamples);

				const int retrieved = (int)stretcher->retrieve(renderPointers.get(), (size_t)chunkSize);
				if (retrieved > 0)
				{
					renderedSamples += retrieved;
					continue;
				}
			}
		}

		int requiredSamples = jmax<int>((int)stretcher->getSamplesRequired(), 1);
		requiredSamples = jmin(requiredSamples, stretchInputBuffer.getNumSamples());

		stretchInputBuffer.clear();

		int writeOffset = 0;
		if (stretchPadRemaining > 0)
		{
			const int padSamples = jmin(requiredSamples, stretchPadRemaining);
			writeOffset += padSamples;
			stretchPadRemaining -= padSamples;
		}

		const int sourceSamplesToWrite = requiredSamples - writeOffset;
		if (sourceSamplesToWrite > 0)
		{
			fillCircularBuffer(stretchInputBuffer, writeOffset, sourceBuffer, stretchInputSample, sourceSamplesToWrite);
			stretchInputSample = (stretchInputSample + sourceSamplesToWrite) % sourceNumSamples;
		}

		HeapBlock<const float*> readPointers(numChannels);
		for (int channel = 0; channel < numChannels; ++channel) readPointers[channel] = stretchInputBuffer.getReadPointer(channel);

		stretcher->process(readPointers.get(), (size_t)requiredSamples, false);
	}

	return renderedSamples == blockSize;
}

void AudioLooperTrack::updateStretch(bool force)
{
	LooperTrack::updateStretch(force);

	if (isCurrentlyLoadingData || Engine::mainEngine->isLoadingFile) return;

	if (bpmAtRecord == 0 || (!isPlaying(true) && !force) || stretch == 1)
	{
		stretchSample = -1;
		resetStretchState(true);
		return;
	}

	stretchSample = -2;
	jumpGhostSample = -1;
	stretchedBuffer.clear();

	if (stretcher == nullptr) rebuildStretcher();
	else stretcher->setTimeRatio(stretch);
}

void AudioLooperTrack::updateReverted()
{
	if (bufferNumSamples <= 0)
	{
		revertedBuffer.clear();
		return;
	}

	revertedBuffer.setSize(numChannels, bufferNumSamples, false, true);

	for (int channel = 0; channel < numChannels; channel++)
	{
		const float* source = buffer.getReadPointer(channel);
		float* dest = revertedBuffer.getWritePointer(channel);

		for (int sample = 0; sample < bufferNumSamples; sample++)
		{
			dest[sample] = source[bufferNumSamples - 1 - sample];
		}
	}

	if (stretch != 1)
	{
		rebuildStretcher();
	}
}

void AudioLooperTrack::stopPlaying()
{
	if (antiClickFadeBeforeStop)
	{
		LooperTrack::stopPlaying();
		resetStretchState(true);
	}
	else
	{
		antiClickFadeBeforeStop = true;
	}
}


void AudioLooperTrack::clearBuffer(bool setIdle)
{
	buffer.clear();
	revertedBuffer.clear();
	resetStretchState(true);
	LooperTrack::clearBuffer(setIdle);
}


void AudioLooperTrack::clearTrack()
{
	if (!isPlaying(false) || antiClickFadeBeforeClear)
	{
		LooperTrack::clearTrack();
	}
	else
	{
		antiClickFadeBeforeClear = true;
	}
}

void AudioLooperTrack::startRecordingInternal()
{
	antiClickFadeBeforeClear = false;

	int recNumSamples = looper->processor->getSampleRate() * 60; // 1 min rec samples
	updateBufferSize(recNumSamples);

	//Store a snapshot of the ring buffer that will be faded at the end of the recorded buffer
	int fadeNumSamples = audioLooper->getFadeNumSamples();
	if (fadeNumSamples > 0)
	{
		preRecBuffer.setSize(buffer.getNumChannels(), fadeNumSamples, false, true);
		audioLooper->ringBuffer->readSamples(preRecBuffer, fadeNumSamples);
	}
}

void AudioLooperTrack::finishRecordingAndPlayInternal()
{
	Transport::Quantization q = looper->getQuantization();
	Transport::Quantization fillMode = looper->getFreeFillMode();

	if (isRecording(false)) updateBufferSize(bufferNumSamples); //update size of buffer, will fill with silence if buffer is larger than what has been recorded

	if (freeRecStartOffset > 0 && q == Transport::FREE && fillMode != Transport::FREE) //This is made to align recorded content to bar/beat by offsetting the data with freeRecStartOffset
	{
		AudioBuffer<float> tmpBuffer(1, buffer.getNumSamples());
		for (int i = 0; i < buffer.getNumChannels(); i++)
		{
			int bufferMinusOffsetSamples = buffer.getNumSamples() - freeRecStartOffset;

			tmpBuffer.copyFrom(0, 0, buffer.getReadPointer(i, bufferMinusOffsetSamples), freeRecStartOffset); //copies end to back
			tmpBuffer.copyFrom(0, freeRecStartOffset, buffer.getReadPointer(i, 0), bufferMinusOffsetSamples);
			buffer.copyFrom(i, 0, tmpBuffer.getReadPointer(0), buffer.getNumSamples());
		}
	}


	if (q != Transport::FREE || (q == Transport::FREE && fillMode == Transport::FREE))
	{
		//fade with ring buffer using looper fadeTimeMS
		int fadeNumSamples = audioLooper->getFadeNumSamples();

		if (fadeNumSamples > 0)
		{
			int cropFadeNumSamples = jmin(bufferNumSamples, fadeNumSamples);
			int bufferStartSample = bufferNumSamples - cropFadeNumSamples;

			int preRecStartSample = preRecBuffer.getNumSamples() - cropFadeNumSamples;

			buffer.applyGainRamp(bufferStartSample, cropFadeNumSamples, 1, 0);

			for (int i = 0; i < buffer.getNumChannels(); i++)
			{
				buffer.addFromWithRamp(i, bufferStartSample, preRecBuffer.getReadPointer(i, preRecStartSample), cropFadeNumSamples, 0, 1);
			}

			preRecBuffer.clear();
		}
	}

	stretch = 1; // reset stretch
	updateReverted();

}

void AudioLooperTrack::retroRecAndPlayInternal()
{
	antiClickFadeBeforeClear = false; //force after rec to avoid clear the track

	updateBufferSize(bufferNumSamples); //update size of buffer, will fill with silence if buffer is larger than what has been recorded


	//fade with ring buffer using looper fadeTimeMS

	int fadeNumSamples = audioLooper->getFadeNumSamples();
	audioLooper->retroRingBuffer->readSamples(buffer, bufferNumSamples, 0);

	if (fadeNumSamples > 0)
	{
		preRecBuffer.setSize(buffer.getNumChannels(), fadeNumSamples, false, true);
		audioLooper->retroRingBuffer->readSamples(preRecBuffer, fadeNumSamples, bufferNumSamples);

		int cropFadeNumSamples = jmin(bufferNumSamples, fadeNumSamples);
		int bufferStartSample = bufferNumSamples - cropFadeNumSamples;

		int preRecStartSample = preRecBuffer.getNumSamples() - cropFadeNumSamples;

		buffer.applyGainRamp(bufferStartSample, cropFadeNumSamples, 1, 0);

		for (int i = 0; i < buffer.getNumChannels(); i++)
		{
			buffer.addFromWithRamp(i, bufferStartSample, preRecBuffer.getReadPointer(i, preRecStartSample), cropFadeNumSamples, 0, 1);
		}

		preRecBuffer.clear();
	}

	stretch = 1; // reset stretch
	updateReverted();
}

void AudioLooperTrack::processBlock(AudioBuffer<float>& inputBuffer, AudioBuffer<float>& outputBuffer, int numMainChannels, bool outputIfRecording)
{
	int blockSize = inputBuffer.getNumSamples();
	int trackChannel = numMainChannels + index;
	bool outputToMainTrack = false;
	bool outputToSeparateTrack = outputBuffer.getNumChannels() > trackChannel;

	TrackState s = trackState->getValueDataAsEnum<TrackState>();

	bool transportIsPlaying = Transport::getInstance()->isCurrentlyPlaying->boolValue();

	if (s == WILL_RECORD
		&& (!transportIsPlaying || looper->getQuantization() == Transport::FREE)
		&& looper->firstRecVolumeThreshold->enabled)
	{
		float mag = inputBuffer.getMagnitude(0, blockSize);

		float decibels = Decibels::gainToDecibels(mag, -100.f);
		float normDecibels = jmap(decibels, -100.f, 6.f, 0.f, 1.f);

		if (normDecibels >= looper->firstRecVolumeThreshold->floatValue())
		{
			startRecording();
			return;
		}
	}

	if (buffer.getNumSamples() == 0)
	{
		return;
	}



	if (isRecording(false))
	{
		if (!finishRecordLock)
		{
			if (buffer.getNumSamples() >= curSample + blockSize)
			{
				for (int i = 0; i < numChannels; i++)
				{
					buffer.copyFrom(i, curSample, inputBuffer, i, 0, blockSize);
				}
				if (outputIfRecording) outputToMainTrack = true;
			}
			else
			{
				jassertfalse; //loop is too long
				LOGWARNING("Record is too long, clear track");
				clearTrack();
			}

		}
	}

	if (reverted->boolValue() && (firstPlayAfterRecord || revertedBuffer.getNumSamples() != bufferNumSamples))
	{
		updateReverted();
	}

	bool isReallyPlaying = isPlaying(false);
	//bool forceForPauseAntiClick = isReallyPlaying && !transportIsPlaying && antiClickFadeBeforePause;

	if (isReallyPlaying)
	{
		if (playQuantization != Transport::FREE)
		{
			if (!transportIsPlaying && !antiClickFadeBeforePause)
			{
				outputToSeparateTrack = false;
				outputToMainTrack = false;
			}
			else
			{
				outputToMainTrack = true;
				if (transportIsPlaying) antiClickFadeBeforePause = true;
			}
		}
		else
		{
			outputToMainTrack = true;
		}
	}

	AudioBuffer<float>* sourceBuffer = &buffer;
	if (reverted->boolValue() && revertedBuffer.getNumSamples() == bufferNumSamples)
	{
		sourceBuffer = &revertedBuffer;
	}

	const bool useRealtimeStretch = stretch != 0 && stretch != 1 && stretcher != nullptr;
	AudioBuffer<float>* targetBuffer = sourceBuffer;
	int totalSamples = useRealtimeStretch ? stretchedNumSamples : bufferNumSamples;

	if ((outputToMainTrack || outputToSeparateTrack) && (curSample < totalSamples))
	{
		if (outputToSeparateTrack)
		{
			outputBuffer.clear(trackChannel, 0, blockSize);
		}

		float vol = getGain();

		if (firstPlayAfterRecord)
		{
			prevGain = 0;
			//			firstPlayAfterRecord = false;
		}
		else if (antiClickFadeBeforeClear || antiClickFadeBeforeStop)
		{
			vol = 0;
		}
		else if (isReallyPlaying && !transportIsPlaying && antiClickFadeBeforePause)
		{
			vol = 0;
		}

		if (firstPlayAfterStop || s == WILL_STOP)
		{
			int fadeReadSample = jumpGhostSample >= 0 ? jumpGhostSample : curSample;

			int fadeSamples = looper->playStopFadeMS->intValue() * blockSize;
			if (firstPlayAfterStop || WILL_STOP)
			{
				if (fadeReadSample < fadeSamples) vol *= fadeReadSample * 1.0 / fadeSamples;
			}

			if (s == WILL_STOP)
			{
				if (fadeReadSample > totalSamples - fadeSamples) vol *= (totalSamples - fadeReadSample) * 1.0 / fadeSamples;
			}
		}



		if (!useRealtimeStretch && jumpGhostSample > 0 && jumpGhostSample + blockSize < totalSamples)
		{
			firstPlayAfterStop = false;

			for (int i = 0; i < numChannels; i++)
			{
				if (outputToMainTrack)
				{
					outputBuffer.addFromWithRamp(i, 0, targetBuffer->getReadPointer(i, jumpGhostSample), blockSize, vol, 0);
				}

				if (outputToSeparateTrack)
				{
					outputBuffer.addFromWithRamp(trackChannel, 0, targetBuffer->getReadPointer(i, jumpGhostSample), blockSize, vol, 0);
				}

				//rmsVal = jmax(rmsVal, buffer.getMagnitude(i, curReadSample, blockSize));
			}

			prevGain = 0;
			jumpGhostSample = -1;
		}

		int targetSample = curSample;
		if (useRealtimeStretch)
		{
			renderStretchedBlock(*sourceBuffer, blockSize);
			targetBuffer = &rtStretchBuffer;
			targetSample = 0;
		}

		//DBG("Play here " << targetSample << ",prevGain " << prevGain << " / " << (int)antiClickFadeBeforePause);

		for (int i = 0; i < numChannels; i++)
		{
			if (outputToMainTrack)
			{
				outputBuffer.addFromWithRamp(i, 0, targetBuffer->getReadPointer(i, targetSample), blockSize, prevGain, vol);
			}

			if (outputToSeparateTrack)
			{
				outputBuffer.addFromWithRamp(trackChannel, 0, targetBuffer->getReadPointer(i, targetSample), blockSize, prevGain, vol);
			}



			//rmsVal = jmax(rmsVal, buffer.getMagnitude(i, curReadSample, blockSize));
		}

		//rmsVal = buffer.getMagnitude(curReadSample, blockSize);

		//float curVal = rms->floatValue();
		//float targetVal = rms->getLerpValueTo(rmsVal, rmsVal > curVal ? .8f : .2f);
		//rms->setValue(targetVal);

		//updateRMS(buffer, -1, curReadSample, blockSize);

		prevGain = vol;
	}
	else
	{
		//rms->setValue(0);
	}


	if (antiClickFadeBeforeClear)
	{
		clearTrack();
		antiClickFadeBeforeClear = false;
	}
	else if (antiClickFadeBeforeStop)
	{
		stopPlaying();
		antiClickFadeBeforeStop = false;
	}
	else if (isReallyPlaying && !transportIsPlaying && antiClickFadeBeforePause)
	{
		firstPlayAfterStop = true;
		antiClickFadeBeforePause = false;
		curSample = 0;
		loopProgression->setValue(0);
		loopBar->setValue(0);
		loopBeat->setValue(0);
	}
	else
	{
		processTrack(blockSize, false);// forceForPauseAntiClick);
	}
}

void AudioLooperTrack::onContainerParameterChanged(Parameter* p)
{
	LooperTrack::onContainerParameterChanged(p);

	if (p == reverted)
	{
		updateReverted();
	}
	else if (p == rbPreserveFormants || p == rbSmoothing || p == rbEngine || p == rbTransients || p == rbWindowSize)
	{
		if (stretch != 1) rebuildStretcher();
	}
}

void AudioLooperTrack::loadSampleFile(File dir)
{

	Array<File> files = dir.findChildFiles(File::findFiles, false, String(index + 1) + "*.wav");

	File f = files.isEmpty() ? File() : files[0];

	if (!f.existsAsFile()) return;

	AudioFormatManager format;
	format.registerFormat(new WavAudioFormat(), true);

	auto is = f.createInputStream();
	auto reader = format.createReaderFor(f);
	if (reader == nullptr) return;

	int fileLength = reader->lengthInSamples;
	int blockPerfectLength = Transport::getInstance()->getBlockPerfectNumSamples(fileLength);
	buffer.setSize(numChannels, blockPerfectLength);
	reader->read(buffer.getArrayOfWritePointers(), numChannels, 0, blockPerfectLength);


	LooperNode::LoadQuantizMode lqm = looper->quantizationOnLoad->getValueDataAsEnum<LooperNode::LoadQuantizMode>();
	int numQuantiz = looper->loadQuantizationValue->intValue();


	switch (lqm)
	{
	case LooperNode::FROM_LOOPER:
	{
		playQuantization = looper->getQuantization();
		numBeats = numQuantiz;
		if (playQuantization == Transport::BAR)
		{
			numBeats *= Transport::getInstance()->beatsPerBar->intValue();
		}
		else if (playQuantization == Transport::FIRSTLOOP)
		{
			numBeats *= Transport::getInstance()->firstLoopBeats->intValue();
			playQuantization = Transport::BEAT;
		}
	}
	break;

	case LooperNode::FROM_FILE:
	{
		StringArray fSplit = StringArray::fromTokens(f.getFileNameWithoutExtension(), "_", "");
		String infos = reader->metadataValues[WavAudioFormat::riffInfoTitle];
		StringArray infoSplit;
		infoSplit.addTokens(infos, ";", "");

		if (fSplit.size() >= 3)
		{
			if (fSplit[2] == "bar") playQuantization = Transport::BAR;
			else if (fSplit[2] == "beat") playQuantization = Transport::BEAT;
			else if (fSplit[2] == "free") playQuantization = Transport::FREE;

			if (playQuantization != Transport::FREE)
			{
				bpmAtRecord = 0;
				if (fSplit.size() >= 2)
				{
					numBeats = fSplit[1].getIntValue();
				}
				else
				{
					numBeats = Transport::getInstance()->getBeatForSamples(blockPerfectLength);
					playQuantization = Transport::BEAT;
					//numBeats = numBars * Transport::getInstance()->beatsPerBar->intValue();
				}

			}

			LOG("Loading quantization from file name : " << fSplit[2] << " " << fSplit[3]);
		}
		else if (infoSplit.size() >= 3)
		{
			bpmAtRecord = infoSplit[0].getFloatValue();
			numBeats = infoSplit[1].getIntValue();
			playQuantization = (Transport::Quantization)infoSplit[2].getIntValue();
			LOG("Loading quantization from metadata " << infos);
		}
		else
		{
			LOGWARNING("This loop file has not been saved by the looper. Trying my best to make something out of it.");
			playQuantization = Transport::BAR;
		}
	}
	break;


	case LooperNode::BAR:
		playQuantization = Transport::BAR;
		numBeats = numQuantiz * Transport::getInstance()->beatsPerBar->intValue();
		break;

	case LooperNode::BEAT:
		playQuantization = Transport::BEAT;
		numBeats = numQuantiz;
		break;

	case LooperNode::FREE:
		playQuantization = Transport::FREE;
		numBeats = 0;
		break;

	}


	curSample = 0;
	bufferNumSamples = buffer.getNumSamples();
	updateReverted();

	trackState->setValueWithData(TrackState::STOPPED);
	updateStretch(true);

	antiClickFadeBeforeClear = false;

	NLOG(niceName, "Loaded track, " << numBeats << " beats, bpm : " << bpmAtRecord << ", quantization : " << (int)playQuantization);

	delete reader;
}

void AudioLooperTrack::saveSampleFile(File dir)
{
	File f = dir.getChildFile(String(index + 1) + ".wav");
	if (f.existsAsFile()) f.deleteFile();
	if (buffer.getNumSamples() == 0) return;

	WavAudioFormat format;
	auto os = f.createOutputStream();
	StringPairArray metaData;
	metaData.set(WavAudioFormat::riffInfoTitle, String(bpmAtRecord) + ";" + String(numBeats) + ";" + String((int)playQuantization));

	auto writer = format.createWriterFor(os.get(), looper->processor->getSampleRate(), numChannels, 16, metaData, 0);
	writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
	writer->flush();

	os.release();
	delete writer;
}

bool AudioLooperTrack::hasContent(bool includeRecordPhase) const
{
	if (!LooperTrack::hasContent(includeRecordPhase)) return false;
	TrackState s = trackState->getValueDataAsEnum<TrackState>();
	if (s == PLAYING && antiClickFadeBeforeClear) return false;
	return true;
}
