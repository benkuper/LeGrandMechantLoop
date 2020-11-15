/*
  ==============================================================================

    IONodeViewUI.cpp
    Created: 15 Nov 2020 11:39:57pm
    Author:  bkupe

  ==============================================================================
*/

#include "IONodeViewUI.h"

InputNodeViewUI::InputNodeViewUI(GenericAudioNode<AudioInputProcessor>* n) :
    GenericAudioNodeViewUI(n)
{
}

InputNodeViewUI::~InputNodeViewUI()
{
}

OutputNodeViewUI::OutputNodeViewUI(GenericAudioNode<AudioOutputProcessor>* n) :
    GenericAudioNodeViewUI(n)
{
}

OutputNodeViewUI::~OutputNodeViewUI()
{
}
