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

class IOChannelUI :
    public Component
{
public:
    IOChannelUI(IOChannel* ioChannel);
    ~IOChannelUI() {}

    IOChannel* ioChannel;

    std::unique_ptr<FloatSliderUI> gainUI;
    std::unique_ptr<RMSSliderUI> rmsUI;
    std::unique_ptr<BoolButtonToggleUI> activeUI;

    void paint(Graphics& g) override;
    void resized() override;
};

class IONodeViewUI :
    public NodeViewUI<IONode>
{
public:
    IONodeViewUI(IONode * n);
    virtual ~IONodeViewUI();

    OwnedArray<IOChannelUI> channelsUI;
    
    void nodeInputsChanged() override;
    void nodeOutputsChanged() override;

    void updateUI();

    void resizedInternalContentNode(Rectangle<int> &r) override;

};