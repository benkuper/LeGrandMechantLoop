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

class IONodeViewUI :
    public GenericNodeViewUI<IOProcessor>
{
public:
    IONodeViewUI(GenericNode<IOProcessor> * n);
    virtual ~IONodeViewUI();

    OwnedArray<FloatSliderUI> rmsUI;
    OwnedArray<FloatSliderUI> gainUI;
    
    void nodeInputsChanged() override;
    void nodeOutputsChanged() override;

    void updateUI();

    void resizedInternalContent(Rectangle<int> &r) override;

};