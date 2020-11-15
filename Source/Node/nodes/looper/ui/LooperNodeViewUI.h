/*
  ==============================================================================

    LooperNodeViewUI.h
    Created: 15 Nov 2020 8:43:15am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "../LooperNode.h"
#include "../../../ui/NodeViewUI.h"

class LooperNodeViewUI :
    public GenericAudioNodeViewUI<LooperProcessor>
{
public:
    LooperNodeViewUI(GenericAudioNode<LooperProcessor>* n);
    ~LooperNodeViewUI();

};
