/*
  ==============================================================================

	AudioLooperTrack.cpp
	Created: 1 Dec 2020 11:15:40pm
	Author:  bkupe

  ==============================================================================
*/

#include "AudioLooperTrack.h"
#include "AudioLooperNode.h"

AudioLooperTrack::AudioLooperTrack(AudioLooperNode* looper, int index, int numChannels) :
	LooperTrack(looper, index),
	audioLooper(looper),
	numChannels(numChannels)
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

void AudioLooperTrack::clearBuffer(bool setIdle)
{
	buffer.clear();
	LooperTrack::clearBuffer(setIdle);
}


void AudioLooperTrack::startRecordingInternal()
{
	int recNumSamples = looper->processor->getSampleRate() * 60; // 1 min rec samples

	updateBufferSize(recNumSamples);

	clearBuffer(false);

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
				buffer.addFromWithRamp(i, bufferStartSample, preRecBuffer.getReadPointer(i,preRecStartSample), cropFadeNumSamples, 0, 1);
			}

			preRecBuffer.clear();
		}
	}

}

void AudioLooperTrack::processBlock(AudioBuffer<float>& inputBuffer, AudioBuffer<float>& outputBuffer, int numMainChannels, bool outputIfRecording)
{
	int blockSize = inputBuffer.getNumSamples();
	int trackChannel = numMainChannels + index;
	bool outputToMainTrack = false;
	bool outputToSeparateTrack = outputBuffer.getNumChannels() > trackChannel;


	TrackState s = trackState->getValueDataAsEnum<TrackState>();

	if (s == WILL_RECORD
		&& (!Transport::getInstance()->isCurrentlyPlaying->boolValue() || looper->getQuantization() == Transport::FREE)
		&& looper->firstRecVolumeThreshold->enabled)
	{
		float mag = inputBuffer.getMagnitude(0, blockSize);
		if (mag >= looper->firstRecVolumeThreshold->floatValue()) startRecording();
	}

	if (buffer.getNumSamples() == 0) return;

	if (isRecording(false))
	{
		if (!finishRecordLock)
		{
			for (int i = 0; i < numChannels; i++)
			{
				buffer.copyFrom(i, curSample, inputBuffer, i, 0, blockSize);
			}
			if (outputIfRecording) outputToMainTrack = true;
		}
	}

	processTrack(blockSize);

	
	if (isPlaying(false))
	{
		if (playQuantization != Transport::FREE && !Transport::getInstance()->isCurrentlyPlaying->boolValue())
		{
			outputToSeparateTrack = false;
			outputToMainTrack = false;
		}
		else
		{
			outputToMainTrack = true;
		}
	}


	if (outputToSeparateTrack)
	{
		outputBuffer.clear(trackChannel, 0, blockSize);
	}

	float vol = getGain();
	if (firstPlay || s == WILL_STOP)
	{
		int fadeSamples = looper->playStopFadeMS->intValue() * blockSize;
		if (firstPlay || WILL_STOP)
		{
			if (curReadSample < fadeSamples) vol *= curReadSample * 1.0 / fadeSamples;
		}
		
		if (s == WILL_STOP)
		{
			if (curReadSample > bufferNumSamples - fadeSamples) vol *= (bufferNumSamples - curReadSample) * 1.0 / fadeSamples;
		}
	}

	if ((outputToMainTrack || outputToSeparateTrack) && (curReadSample <= bufferNumSamples))
	{
		for (int i = 0; i < numChannels; i++)
		{
			if (outputToMainTrack)
			{
				outputBuffer.addFromWithRamp(i, 0, buffer.getReadPointer(i, curReadSample), blockSize, prevGain, vol);
			}

			if (outputToSeparateTrack)
			{
				outputBuffer.addFromWithRamp(trackChannel, 0, buffer.getReadPointer(i, curReadSample), blockSize, prevGain, vol);
				
			}

			//rmsVal = jmax(rmsVal, buffer.getMagnitude(i, curReadSample, blockSize));
		}

		//rmsVal = buffer.getMagnitude(curReadSample, blockSize);

		//float curVal = rms->floatValue();
		//float targetVal = rms->getLerpValueTo(rmsVal, rmsVal > curVal ? .8f : .2f);
		//rms->setValue(targetVal);

		//updateRMS(buffer, -1, curReadSample, blockSize);
	}
	else
	{
		//rms->setValue(0);
	}

	prevGain = vol;
}
