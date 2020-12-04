/*
  ==============================================================================

    AudioRouterNode.h
    Created: 4 Dec 2020 11:55:34am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "../../NodeProcessor.h"

class AudioRouterProcessor :
    public GenericNodeProcessor
{
public:
    AudioRouterProcessor(Node * n);
    ~AudioRouterProcessor();

    IntParameter* numChannelsPerGroup;
    
    IntParameter * currentInput;
    IntParameter * currentOutput;
    IntParameter * transitionMS;

    int previousInput;
    int previousOutput;

    uint32 timeAtTransition;

    AudioBuffer<float> tmpBuffer;

    void autoSetNumInputs() override;
    void autoSetNumOutputs() override;

    void updateInputsFromNodeInternal() override;
    void updateOutputsFromNodeInternal() override;
    
    void onContainerParameterChanged(Parameter* p) override;

    void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;
    static const String getTypeStringStatic() { return "Audio Router"; }

    NodeViewUI* createNodeViewUI() override;
};