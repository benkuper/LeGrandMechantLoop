/*
  ==============================================================================

    VSTNodeViewUI.h
    Created: 15 Nov 2020 11:41:16pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "../VSTNode.h"
#include "../../../ui/NodeViewUI.h"

class VSTNodeViewUI :
    public GenericAudioNodeViewUI<VSTProcessor>
{
public:
    VSTNodeViewUI(GenericAudioNode<VSTProcessor>* n);
    ~VSTNodeViewUI();

};