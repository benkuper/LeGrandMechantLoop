/*
  ==============================================================================

	MetronomeNode.cpp
	Created: 8 Jun 2023 7:55:44am
	Author:  bkupe

  ==============================================================================
*/

#include "Node/NodeIncludes.h"
#include "MetronomeNode.h"

MetronomeNode::MetronomeNode(var params) :
	Node(getTypeString(), params, false, true, false),
	currentTransport(nullptr)
{

	WavAudioFormat wavFormat;

	Array<const char*> ticData{ BinaryData::tic1_wav, BinaryData::tic2_wav, BinaryData::tic3_wav  };
	Array<int> ticSizes{ BinaryData::tic1_wavSize, BinaryData::tic2_wavSize, BinaryData::tic3_wavSize };


	for (int i = 0; i < ticData.size(); i++)
	{
		MemoryInputStream* ts(new MemoryInputStream(ticData[i], ticSizes[i], true));
		AudioFormatReader* reader(wavFormat.createReaderFor(ts, true));

		AudioFormatReaderSource* newSource = new AudioFormatReaderSource(reader, true);
		AudioTransportSource* transport = new AudioTransportSource();


		transport->setSource(newSource, 0, nullptr, reader->sampleRate, reader->numChannels);
		ticTransports.add(transport);
		ticReaders.add(newSource);
	}

	setAudioOutputs(1);

	Transport::getInstance()->addTransportListener(this);
}

MetronomeNode::~MetronomeNode()
{
	if (Transport::getInstanceWithoutCreating() != nullptr) Transport::getInstance()->removeTransportListener(this);

}

void MetronomeNode::beatChanged(bool isNewBar, bool isFirstLoopBeat)
{
	if (!Transport::getInstance()->isCurrentlyPlaying->boolValue()) return;
    if(!enabled->boolValue()) return;
    
	int index = isFirstLoopBeat ? 0 : isNewBar ? 1 : 2;
	currentTransport = ticTransports[index];

	if (currentTransport != nullptr)
	{
		currentTransport->setPosition(0);
		currentTransport->start();
	}
}

void MetronomeNode::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock)
{
	for (auto& t : ticTransports) t->prepareToPlay(maximumExpectedSamplesPerBlock, sampleRate);
}

void MetronomeNode::playStateChanged(bool isPlaying, bool forceRestart)
{
    if(!enabled->boolValue()) return;
    
	if (!isPlaying)
	{
		if (currentTransport != nullptr)
		{
			currentTransport->stop();
			currentTransport = nullptr;
		}
	}
	else
	{
		if (currentTransport == nullptr) currentTransport = ticTransports[0];
		currentTransport->setPosition(0);
		currentTransport->start();
	}
}

void MetronomeNode::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    if (!Transport::getInstance()->isCurrentlyPlaying->boolValue()) return;
    if(!enabled->boolValue()) return;
    
	buffer.clear();
	if (currentTransport != nullptr) currentTransport->getNextAudioBlock(AudioSourceChannelInfo(buffer));
}

