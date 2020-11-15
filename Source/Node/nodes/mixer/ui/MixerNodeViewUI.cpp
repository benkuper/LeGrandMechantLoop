/*
  ==============================================================================

    MixerNodeViewUI.cpp
    Created: 15 Nov 2020 11:40:42pm
    Author:  bkupe

  ==============================================================================
*/

#include "MixerNodeViewUI.h"

MixerNodeViewUI::MixerNodeViewUI(GenericAudioNode<MixerProcessor>* n) :
    GenericAudioNodeViewUI(n)
{
    LOG("Mixer node view !");
}

MixerNodeViewUI::~MixerNodeViewUI()
{
}
