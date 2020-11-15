/*
  ==============================================================================

    SpatNodeViewUI.h
    Created: 15 Nov 2020 11:41:00pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "../SpatNode.h"
#include "../../../ui/NodeViewUI.h"

class SpatNodeViewUI :
    public GenericAudioNodeViewUI<SpatProcessor>
{
public:
    SpatNodeViewUI(GenericAudioNode<SpatProcessor>* n);
    ~SpatNodeViewUI();

};