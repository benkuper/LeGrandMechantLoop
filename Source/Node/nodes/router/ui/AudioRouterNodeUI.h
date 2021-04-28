/*
  ==============================================================================

    AudioRouterNodeUI.h
    Created: 4 Dec 2020 11:58:03am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

class AudioRouterNodeViewUI :
    public NodeViewUI<AudioRouterNode>
{
public:
    AudioRouterNodeViewUI(AudioRouterNode * n);
    ~AudioRouterNodeViewUI();

    std::unique_ptr<ParameterEditor> currentInputUI;
    std::unique_ptr<ParameterEditor> currentOutputUI;

    void resizedInternalContentNode(Rectangle<int>& r) override;
};