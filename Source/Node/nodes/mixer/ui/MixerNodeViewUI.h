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

    class OutputGainLine :
        public Component
    {
    public:
        OutputGainLine(MixerNode * node, OutputLineCC * outputLine);

        MixerNode* node;
        OutputLineCC * outputLine;

        OwnedArray<VolumeControlUI> itemsUI;
        std::unique_ptr<VolumeControlUI> outUI;
        std::unique_ptr<BoolButtonToggleUI> exclusiveUI;

        void rebuild();

        void paint(Graphics& g) override;
        void resized() override;
    };

    OwnedArray<OutputGainLine> gainLines;

    void nodeInputsChanged() override;
    void nodeOutputsChanged() override;

    void updateLines();

    void viewFilterUpdated() override;

    void paint(Graphics& g) override;
    void resizedInternalContentNode(Rectangle<int> &r) override;
};