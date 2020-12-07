/*
  ==============================================================================

    ContainerNodeUI.h
    Created: 3 Dec 2020 9:43:14am
    Author:  bkupe

  ==============================================================================
*/

#pragma once
#include "../ContainerNode.h"
#include "../../../ui/NodeViewUI.h"

class ContainerNodeViewUI :
    public NodeViewUI<ContainerNode>
{
public:
    ContainerNodeViewUI(ContainerNode * node);
    ~ContainerNodeViewUI();

    std::unique_ptr<ImageButton> editHeaderBT;
    
    void resizedInternalHeader(Rectangle<int>& r) override;

    void buttonClicked(Button*) override;
};