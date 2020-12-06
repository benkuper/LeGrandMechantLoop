/*
  ==============================================================================

    AudioRouterNode.h
    Created: 4 Dec 2020 11:55:34am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "../../Node.h"

class AudioRouterNode :
    public Node
{
public:
    AudioRouterNode(var params = var());
    ~AudioRouterNode();

    IntParameter* numChannelsPerGroup;
    
    IntParameter * currentInput;
    IntParameter * currentOutput;
    IntParameter * transitionMS;

    int previousInput;
    int previousOutput;

    uint32 timeAtTransition;

    AudioBuffer<float> tmpBuffer;

    void autoSetNumAudioInputs() override;
    void autoSetNumAudioOutputs() override;

    void updateAudioInputsInternal() override;
    void updateAudioOutputsInternal() override;
    
    void onContainerParameterChangedInternal(Parameter* p) override;

    void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;
    
    String getTypeString() const override { return getTypeStringStatic(); }
    static const String getTypeStringStatic() { return "Audio Router"; }

    BaseNodeViewUI* createViewUI() override;
};