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
    public NodeViewUI<MixerNode>
{
public:
    MixerNodeViewUI(MixerNode* n);
    ~MixerNodeViewUI();

    class InputGainLine :
        public Component
    {
    public:
        InputGainLine(ControllableContainer* gainsCC, int index);

        int index;
        ControllableContainer* itemsCC;
        OwnedArray<VolumeControlUI> itemsUI;

        void rebuild();

        void paint(Graphics& g) override;
        void resized() override;
    };

    OwnedArray<InputGainLine> gainLines;

    OwnedArray<VolumeControlUI> outItemsUI;

    Rectangle<int> outRect;

    void nodeInputsChanged() override;
    void nodeOutputsChanged() override;

    void updateLines();
    void rebuildOutLine();

    void paint(Graphics& g) override;
    void resizedInternalContentNode(Rectangle<int> &r) override;
};