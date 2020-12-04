/*
  ==============================================================================

    AudioRouterNodeUI.h
    Created: 4 Dec 2020 11:58:03am
    Author:  bkupe

  ==============================================================================
*/

#pragma once
#include "../../../ui/NodeViewUI.h"
#include "../AudioRouterNode.h"

class AudioRouterNodeViewUI :
    public GenericNodeViewUI<AudioRouterProcessor>
{
public:
    AudioRouterNodeViewUI(GenericNode<AudioRouterProcessor>* n);
    ~AudioRouterNodeViewUI();

    std::unique_ptr<ParameterEditor> currentInputUI;
    std::unique_ptr<ParameterEditor> currentOutputUI;

    void resizedInternalContentNode(Rectangle<int>& r) override;
};