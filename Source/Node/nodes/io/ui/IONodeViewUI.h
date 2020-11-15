/*
  ==============================================================================

    IONodeViewUI.h
    Created: 15 Nov 2020 11:39:57pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once
#include "../IONode.h"
#include "../../../ui/NodeViewUI.h"

class InputNodeViewUI :
    public GenericAudioNodeViewUI<AudioInputProcessor>
{
public:
    InputNodeViewUI(GenericAudioNode<AudioInputProcessor> * n);
    ~InputNodeViewUI();

};

class OutputNodeViewUI :
    public GenericAudioNodeViewUI<AudioOutputProcessor>
{
public:
    OutputNodeViewUI(GenericAudioNode<AudioOutputProcessor>* n);
    ~OutputNodeViewUI();

};