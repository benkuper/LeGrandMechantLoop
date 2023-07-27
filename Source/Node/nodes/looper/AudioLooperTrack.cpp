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
	antiClickFadeBeforeClear(true), //needs that otherwise first clear doesn't work
	antiClickFadeBeforeStop(false),
	antiClickFadeBeforePause(false)
{
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

void AudioLooperTrack::updateStretch(bool force)
{
	LooperTrack::updateStretch(force);

	if (bpmAtRecord == 0 || (!isPlaying(true) && !force) || stretch == 1)
	{
		stretchedBuffer.clear();
		stretcher.reset();
		return;
	}

	if (stretcher == nullptr)
	{
		stretcher.reset(new RubberBand::RubberBandStretcher(looper->processor->getSampleRate(), numChannels,
			RubberBand::RubberBandStretcher::OptionProcessRealTime
			| RubberBand::RubberBandStretcher::OptionStretchPrecise
			| RubberBand::RubberBandStretcher::OptionFormantPreserved
			| RubberBand::RubberBandStretcher::OptionWindowStandard
			| RubberBand::RubberBandStretcher::OptionTransientsCrisp
			| RubberBand::RubberBandStretcher::OptionChannelsTogether
		));
		stretcher->setMaxProcessSize(looper->processor->getBlockSize());
	}
	else
	{
		//stretcher->reset();
	}

	stretcher->setTimeRatio(stretch);
	stretchedBuffer.setSize(numChannels, stretchedNumSamples);
}

void AudioLooperTrack::stopPlaying()
{
	if (antiClickFadeBeforeStop)
	{
		LooperTrack::stopPlaying();
	}
	else
	{
		antiClickFadeBeforeStop = true;
	}
}


void AudioLooperTrack::clearBuffer(bool setIdle)
{
	buffer.clear();
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
		if (mag >= looper->firstRecVolumeThreshold->floatValue())
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

	AudioBuffer<float>* targetBuffer = &buffer;
	int totalSamples = bufferNumSamples;

	if (stretch != 1 && stretchSample == -2)
	{
		targetBuffer = &stretchedBuffer;
		totalSamples = stretchedNumSamples;
	}

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



		if (jumpGhostSample > 0 && jumpGhostSample + blockSize < totalSamples)
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
		if (stretch != 0 && stretch != 1 && stretcher != nullptr)
		{
			if (stretchSample == -2) //use stretchedBuffer instead of calculating
			{
				targetBuffer = &stretchedBuffer;
			}
			else
			{
				if (stretchSample == 0 && curSample == 0) //set from handleNewBeat in LooperTrack
				{
					DBG("Start storing stretched here " << curSample << "/" << Transport::getInstance()->getRelativeBarSamples());
					stretcher->reset();
				}

				rtStretchBuffer.clear();
				rtStretchBuffer.setSize(numChannels, blockSize);
				AudioBuffer<float> tmpBuffer(buffer.getNumChannels(), blockSize);
				auto readPointers = tmpBuffer.getArrayOfReadPointers();

				while (stretcher->available() < blockSize)
				{
					for (int i = 0; i < numChannels; i++) tmpBuffer.copyFrom(i, 0, buffer.getReadPointer(i, curSample), blockSize);

					stretcher->process(readPointers, tmpBuffer.getNumSamples(), false);

					curSample += blockSize;
					if (curSample >= buffer.getNumSamples()) curSample = 0;
				}

				stretcher->retrieve(rtStretchBuffer.getArrayOfWritePointers(), rtStretchBuffer.getNumSamples());

				if (stretchSample >= 0)
				{
					for (int i = 0; i < numChannels; i++) stretchedBuffer.copyFrom(i, stretchSample, rtStretchBuffer, i, 0, rtStretchBuffer.getNumSamples());

					stretchSample += rtStretchBuffer.getNumSamples();

					if (stretchSample >= stretchedNumSamples) stretchSample = -2; //if a full loop is written, set -2 to start using the stored buffer
				}

				targetBuffer = &rtStretchBuffer;
				targetSample = 0;
			}
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

	String infos = reader->metadataValues[WavAudioFormat::riffInfoTitle];

	StringArray infoSplit;
	infoSplit.addTokens(infos, ";", "");

	if (infoSplit.size() < 3)
	{
		LOGWARNING("This loop file has not been saved by the looper. Trying my best to make something out of it.");

		StringArray fSplit = StringArray::fromTokens(f.getFileNameWithoutExtension(), "_", "");

		if (fSplit.size() >= 3)
		{
			if (fSplit[2] == "bar") playQuantization = Transport::BAR;
			else if (fSplit[2] == "beat") playQuantization = Transport::BEAT;
			else if (fSplit[2] == "free") playQuantization = Transport::FREE;
		}
		else playQuantization = Transport::BAR;

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
	}
	else
	{
		bpmAtRecord = infoSplit[0].getFloatValue();
		numBeats = infoSplit[1].getIntValue();
		playQuantization = (Transport::Quantization)infoSplit[2].getIntValue();
	}



	curSample = 0;
	bufferNumSamples = buffer.getNumSamples();

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
