/*
  ==============================================================================

    IONode.h
    Created: 15 Nov 2020 8:42:54am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "../../Node.h"

class IONode :
    public Node
{
public:
    IONode(StringRef name = "IO", Type type = CUSTOM, NodeAudioProcessor * processor = nullptr, var params = var());
    virtual ~IONode();
};


class AudioInputNode :
    public IONode
{
public:
    AudioInputNode(var params = var());
    ~AudioInputNode();

    String getTypeString() const override { return getTypeStringStatic(); }
    static const String getTypeStringStatic() { return "Audio Input"; }

    class InputProcessor :
        public NodeAudioProcessor
    {
    public:
        InputProcessor(AudioInputNode * node);
        ~InputProcessor();

        AudioInputNode* inputNode;

        void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;
    };

    InputProcessor * inputProcessor;

    static AudioInputNode* create(var params) { return new AudioInputNode(params); }
};

class AudioOutputNode :
    public IONode
{
public:
    AudioOutputNode(var params = var());
    ~AudioOutputNode();

    class OutputProcessor :
        public NodeAudioProcessor
    {
    public:
        OutputProcessor(AudioOutputNode * node);
        ~OutputProcessor();

        AudioOutputNode* outputNode;

        void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;
    };

    OutputProcessor* outputProcessor;

    String getTypeString() const override { return getTypeStringStatic(); }
    static const String getTypeStringStatic() { return "Audio Output"; }

    static AudioOutputNode* create(var params) { return new AudioOutputNode(params); }
};