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

        void rebuild();

        void paint(Graphics& g) override;
        void resized() override;
    };

    OwnedArray<OutputGainLine> gainLines;
    OwnedArray<BoolButtonToggleUI> exclusivesUI;

    void nodeInputsChanged() override;
    void nodeOutputsChanged() override;

    void updateLines();
    void updateExclusives();
    void viewFilterUpdated() override;

    void paint(Graphics& g) override;
    void resizedInternalContentNode(Rectangle<int> &r) override;
};