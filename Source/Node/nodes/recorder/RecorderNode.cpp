/*
  ==============================================================================

	RecorderNode.cpp
	Created: 27 May 2021 1:28:22pm
	Author:  bkupe

  ==============================================================================
*/

RecorderNode::RecorderNode(var params) :
	Node("Recorder", params, true, false, true, false),
	sampleRate(0),
	stopOnNextSilence(false),
	timeSinceLastVolumeUnderThreshold(-1)
{
	recFolder = addFileParameter("Rec Folder", "Folder to record the audio to");
	recFolder->directoryMode = true;

	baseName = addStringParameter("Base Filename", "The file name to record the session to", "");

	autoRecOnPlay = addBoolParameter("Auto Rec On Play", "If checked, this will automatically start recording when the Transport starts playing", false);
	autoStopOnStop = addBoolParameter("Auto Stop On Stop", "If checked, this will automatically start recording when the Transport starts playing", false);

	stopVolumeThreshold = addFloatParameter("Stop Volume Threshold", "If the volume is below this threshold, it will stop recording", .05, .01, 1);
	stopVolumeThreshold->canBeDisabledByUser = true;

	stopVolumeTime = addFloatParameter("Stop Volume Time", "Time in seconds to wait before stopping recording after the volume is below the threshold", 1, 0, 10);
	stopVolumeTime->defaultUI = FloatParameter::TIME;

	recSeparateFiles = addBoolParameter("Separate Channels", "If checked, this will record one file per channel", false);

	recTrigger = addTrigger("Rec", "Starts or stops recording depending on the current recording state");
	isRecording = addBoolParameter("Is Recording", "Is it currently recording ?", false);
	isRecording->setControllableFeedbackOnly(true);

	Transport::getInstance()->addTransportListener(this);

	backgroundThread.startThread();
}

RecorderNode::~RecorderNode()
{
	Transport::getInstance()->removeTransportListener(this);
	stopRecording();
}

void RecorderNode::startRecording()
{
	stopRecording();
	if (!enabled->boolValue()) return;
	if (sampleRate > 0)
	{
		Array<File> files;
		if (recSeparateFiles->boolValue())
		{
			for (int i = 0; i < getNumAudioInputs(); i++)
			{
				File file = recFolder->getFile().getNonexistentChildFile(baseName->stringValue(), "_channel" + String(i + 1) + ".wav", false);
				files.add(file);
				NLOG(niceName, "Start recording channel " << i + 1 << " to " << file.getFullPathName());
			}
		}
		else
		{
			File file = recFolder->getFile().getNonexistentChildFile(baseName->stringValue(), ".wav", false);
			files.add(file);
			NLOG(niceName, "Start recording mix to " << file.getFullPathName());
		}

		threadedWriters.clear();

		int numChannelsToWrite = recSeparateFiles->boolValue() ? 1 : getNumAudioInputs();

		for (auto& f : files)
		{

			if (auto fileStream = std::unique_ptr<FileOutputStream>(f.createOutputStream()))
			{
				// Now create a WAV writer object that writes to our output stream...
				WavAudioFormat wavFormat;

				if (auto writer = wavFormat.createWriterFor(fileStream.get(), sampleRate, numChannelsToWrite, 16, {}, 0))
				{
					fileStream.release(); // (passes responsibility for deleting the stream to the writer object that is now using it)

					// Now we'll create one of these helper objects which will act as a FIFO buffer, and will
					// write the data to disk on our background thread.
					AudioFormatWriter::ThreadedWriter* w = new AudioFormatWriter::ThreadedWriter(writer, backgroundThread, 32768);
					threadedWriters.add(w);

					// Reset our recording thumbnail
					// thumbnail.reset(writer->getNumChannels(), writer->getSampleRate());
					nextSampleNum = 0;

					// And now, swap over our active writer pointer so that the audio callback will start using it..
					const ScopedLock sl(writerLock);
					activeWriters.add(new std::atomic<AudioFormatWriter::ThreadedWriter*>(w));
				}
			}
		}

		stopOnNextSilence = false;
		isRecording->setValue(true);
	}

}

void RecorderNode::stopRecording()
{
	// First, clear this pointer to stop the audio callback from using our writer object..
	{
		const ScopedLock sl(writerLock);
		activeWriters.clear();
	}

	// Now we can delete the writer object. It's done in this order because the deletion could
	// take a little time while remaining data gets flushed to disk, so it's best to avoid blocking
	// the audio callback while this happens.
	threadedWriters.clear();

	stopOnNextSilence = false;
	NLOG(niceName, "Stop recording");
	isRecording->setValue(false);
}

void RecorderNode::onContainerParameterChangedInternal(Parameter* p)
{
	Node::onContainerParameterChangedInternal(p);
	if (p == enabled) if (!enabled->boolValue()) stopRecording();
}

void RecorderNode::onContainerTriggerTriggered(Trigger* t)
{
	Node::onContainerTriggerTriggered(t);
	if (t == recTrigger)
	{
		if (isRecording->boolValue()) stopRecording();
		else startRecording();
	}
}

void RecorderNode::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock)
{
	this->sampleRate = sampleRate;
}

void RecorderNode::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
	const ScopedLock sl(writerLock);

	if (!isRecording->boolValue()) return;

	if (stopOnNextSilence)
	{
		float volume = buffer.getMagnitude(0, 0, buffer.getNumSamples());
		float decibels = Decibels::gainToDecibels(volume, -100.f);
		float normDecibels = jmap(decibels, -100.f, 6.f, 0.f, 1.f);

		if (normDecibels < stopVolumeThreshold->floatValue())
		{
			float t = Time::getMillisecondCounterHiRes() / 1000.0f;

			if (timeSinceLastVolumeUnderThreshold == -1) timeSinceLastVolumeUnderThreshold = t;
			else if (t > timeSinceLastVolumeUnderThreshold + stopVolumeTime->floatValue())
			{
				stopRecording();
				return;
			}
		}
		else
		{
			timeSinceLastVolumeUnderThreshold = -1;
		}
	}


	bool separate = recSeparateFiles->boolValue();

	AudioSampleBuffer tmpBuffer(1, buffer.getNumSamples()); //for separate

	for (int i = 0; i < activeWriters.size(); i++)
	{
		if (AudioFormatWriter::ThreadedWriter* w = activeWriters[i]->load())
		{
			if (separate)
			{
				tmpBuffer.copyFrom(0, 0, buffer, i, 0, buffer.getNumSamples());
				w->write(tmpBuffer.getArrayOfReadPointers(), buffer.getNumSamples());
			}
			else
			{
				w->write(buffer.getArrayOfReadPointers(), buffer.getNumSamples());
			}

			// Create an AudioBuffer to wrap our incoming data, note that this does no allocations or copies, it simply references our input data
		   // AudioBuffer<float> buffer(const_cast<float**> (inputChannelData), thumbnail.getNumChannels(), numSamples);
			//thumbnail.addBlock(nextSampleNum, buffer, 0, numSamples);
			//nextSampleNum += numSamples;
		}
	}

}

void RecorderNode::playStateChanged(bool isPlaying, bool forceRestart)
{
	if (isPlaying)
	{
		if (enabled->boolValue() && autoRecOnPlay->boolValue() && !forceRestart) startRecording();
	}
	else if (autoStopOnStop->boolValue())
	{
		if (stopVolumeThreshold->enabled && stopVolumeThreshold->floatValue() > 0)
		{
			timeSinceLastVolumeUnderThreshold = -1;
			stopOnNextSilence = true;
		}
		else stopRecording();
	}
}
