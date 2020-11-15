/*
  ==============================================================================

    MixerNodeViewUI.h
    Created: 15 Nov 2020 11:40:42pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once
#include "../MixerNode.h"
#include "../../../ui/NodeViewUI.h"

class MixerNodeViewUI :
    public GenericAudioNodeViewUI<MixerProcessor>
{
public:
    MixerNodeViewUI(GenericAudioNode<MixerProcessor>* n);
    ~MixerNodeViewUI();

};