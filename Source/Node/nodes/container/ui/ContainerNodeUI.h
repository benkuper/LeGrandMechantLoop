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
    public GenericNodeViewUI<ContainerProcessor>
{
public:
    ContainerNodeViewUI(GenericNode<ContainerProcessor> * node);
    ~ContainerNodeViewUI();

    TextButton editBT;
    
    void resizedInternalContentNode(Rectangle<int>& r) override;

    void buttonClicked(Button*) override;
};