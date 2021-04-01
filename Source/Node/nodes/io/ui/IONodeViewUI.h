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
#include "Common/AudioUIHelpers.h"


class IONodeViewUI :
    public NodeViewUI<IONode>
{
public:
    IONodeViewUI(IONode * n);
    virtual ~IONodeViewUI();

    OwnedArray<VolumeControlUI> channelsUI;
    
    void nodeInputsChanged() override;
    void nodeOutputsChanged() override;

    void updateUI();

    void resizedInternalContentNode(Rectangle<int> &r) override;

};