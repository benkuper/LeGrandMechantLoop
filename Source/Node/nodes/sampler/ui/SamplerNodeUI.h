/*
  ==============================================================================

    SamplerNodeUI.h
    Created: 14 Apr 2021 8:42:07am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "../SamplerNode.h"
#include "../../../ui/NodeViewUI.h"

class SamplerNodeViewUI :
    public NodeViewUI<SamplerNode>
{
public:
    SamplerNodeViewUI(SamplerNode* n);
    ~SamplerNodeViewUI();

    std::unique_ptr<ImageButton> editHeaderBT;
    std::unique_ptr<MIDIDeviceParameterUI> midiParamUI;

    MidiKeyboardComponent midiComp;

    void resizedInternalHeader(Rectangle<int>& r) override;
    void resizedInternalContentNode(Rectangle<int>& r) override;
    void controllableFeedbackUpdateInternal(Controllable* c) override;
};
