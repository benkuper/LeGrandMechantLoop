/*
  ==============================================================================

    MappingManagerUI.h
    Created: 3 May 2021 11:39:41am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "MappingUI.h"
#include "../MappingManager.h"

class MappingManagerUI :
    public BaseManagerShapeShifterUI<MappingManager, Mapping, MappingUI>
{
public:
    MappingManagerUI(const String& name);
    ~MappingManagerUI();

    bool isInterestedInDragSource(const DragAndDropTarget::SourceDetails& details) override;
    void itemDropped(const DragAndDropTarget::SourceDetails& details) override;

    static MappingManagerUI* create(const String& name) { return new MappingManagerUI(name); }
};