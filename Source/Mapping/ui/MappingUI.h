/*
  ==============================================================================

    MappingUI.h
    Created: 3 May 2021 11:39:45am
    Author:  bkupe

  ==============================================================================
*/

#pragma once
#include "../Mapping.h"

class MappingUI :
    public BaseItemUI<Mapping>
{
public:
    MappingUI(Mapping * item);
    ~MappingUI();

    std::unique_ptr<ControllableUI> valueUI; //macro or generic
    std::unique_ptr<ControllableUI> outUI;

    void resizedInternalHeader(Rectangle<int>& r) override;

    void updateOutUI();
    void updateValueUI();

    void controllableFeedbackUpdateInternal(Controllable* c) override;

    bool isInterestedInDragSource(const DragAndDropTarget::SourceDetails& details) override;
    void itemDropped(const DragAndDropTarget::SourceDetails& details) override;
};