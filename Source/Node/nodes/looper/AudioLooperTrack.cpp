/*
  ==============================================================================

    AudioLooperTrack.cpp
    Created: 1 Dec 2020 11:15:40pm
    Author:  bkupe

  ==============================================================================
*/

#include "AudioLooperTrack.h"
#include "AudioLooperNode.h"

AudioLooperTrack::AudioLooperTrack(AudioLooperProcessor* looper, int index, int numChannels) :
	LooperTrack(looper, index),
	numChannels(numChannels),
	audioLooper(looper)
{
	volume = addFloatParameter("Gain", "The gain for this track", 1, 0, 2);
	rms = addFloatParameter("RMS", "RMS for this track, for feedback", 0, 0, 1);
	rms->setControllableFeedbackOnly(true);
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

void AudioLooperTrack::clearBuffer()
{
	buffer.clear();
	LooperTrack::clearBuffer();
}

void AudioLooperTrack::startRecordingInternal()
{
	int recNumSamples = looper->getSampleRate() * 60; // 1 min rec samples

	updateBufferSize(recNumSamples);

	clearBuffer();

	//Store a snapshot of the ring buffer that will be faded at the end of the recorded buffer
	int fadeNumSamples = looper->getFadeNumSamples();
	if (fadeNumSamples > 0)
	{
		preRecBuffer.setSize(buffer.getNumChannels(), fadeNumSamples, false);
		audioLooper->ringBuffer->readSamples(preRecBuffer, fadeNumSamples);
	}
}

void AudioLooperTrack::finishRecordingAndPlayInternal()
{
	Transport::Quantization q = looper->getQuantization();
	Transport::Quantization fillMode = looper->getFreeFillMode();

	if (isRecording(false)) updateBufferSize(curSample); //update size of buffer, will fill with silence if buffer is larger than what has been recorded

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
		int fadeNumSamples = looper->getFadeNumSamples();

		if (fadeNumSamples)
		{
			int bufferStartSample = curSample - 1 - fadeNumSamples;

			buffer.applyGainRamp(bufferStartSample, fadeNumSamples, 1, 0);
			for (int i = 0; i < buffer.getNumChannels(); i++)
			{
				buffer.addFromWithRamp(i, bufferStartSample, preRecBuffer.getReadPointer(i), fadeNumSamples, 0, 1);
			}

			preRecBuffer.clear();
		}
	}
}

void AudioLooperTrack::processBlock(AudioBuffer<float>& inputBuffer, AudioBuffer<float>& outputBuffer, int numMainChannels, bool outputIfRecording)
{
	float rmsVal = 0;

	bool outputToMainTrack = false;
	int trackChannel = numMainChannels + index;
	bool outputToSeparateTrack = outputBuffer.getNumChannels() > trackChannel;

	int startReadSample = 0;
	int blockSize = inputBuffer.getNumSamples();

	if (isRecording(false))
	{
		if (!finishRecordLock)
		{
			for (int i = 0; i < numChannels; i++)
			{
				buffer.copyFrom(i, curSample, inputBuffer, i, 0, blockSize);
			}

			curSample += blockSize;
			startReadSample = curSample;

			if (outputIfRecording) outputToMainTrack = true;
		}
	}
	else if (isPlaying(false))
	{
		outputToMainTrack = true;

		if (playQuantization == Transport::FREE)
		{
			if (curPlaySample + blockSize >= buffer.getNumSamples()) curPlaySample = 0;
			startReadSample = curPlaySample;
			curPlaySample += blockSize;
		}
		else //bar, beat
		{
			int curBeat = Transport::getInstance()->getTotalBeatCount() - globalBeatAtStart;
			int trackBeat = curBeat % numBeats;

			int relBeatSamples = Transport::getInstance()->getRelativeBeatSamples();
			startReadSample = Transport::getInstance()->getSamplesForBeat(trackBeat, 0, false) + relBeatSamples;

			loopBeat->setValue(trackBeat);
			loopBar->setValue(floor(trackBeat * 1.0f / Transport::getInstance()->beatsPerBar->intValue()));
		}

		loopProgression->setValue(startReadSample * 1.0f / buffer.getNumSamples());
	}

	if (outputToSeparateTrack)
	{
		outputBuffer.clear(trackChannel, 0, blockSize);
	}

	if ((outputToMainTrack || outputToSeparateTrack) && (startReadSample <= buffer.getNumSamples()))
	{
		for (int i = 0; i < numChannels; i++)
		{
			if (outputToMainTrack)
			{
				outputBuffer.addFrom(i, 0, buffer, i, startReadSample, blockSize, volume->floatValue());
			}

			if (outputToSeparateTrack)
			{
				outputBuffer.addFrom(trackChannel, 0, buffer, i, startReadSample, blockSize, volume->floatValue());
			}

			rmsVal = jmax(rmsVal, buffer.getRMSLevel(i, startReadSample, blockSize));
		}

		float curVal = rms->floatValue();
		float targetVal = rms->getLerpValueTo(rmsVal, rmsVal > curVal ? .8f : .2f);
		rms->setValue(targetVal);
	}
	else
	{
		rms->setValue(0);
	}
}
