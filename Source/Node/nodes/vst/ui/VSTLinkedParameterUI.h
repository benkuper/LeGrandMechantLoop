/*
  ==============================================================================

    VSTLinkedParameterUI.h
    Created: 7 Dec 2020 7:01:39pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "../VSTLinkedParameter.h"

class VSTParameterContainerEditor :
    public GenericControllableContainerEditor
{
public:
    VSTParameterContainerEditor(VSTParameterContainer* vstParamContainer, bool isRoot);
    ~VSTParameterContainerEditor();

    VSTParameterContainer* vstParamContainer;
    std::unique_ptr<ImageButton> addParamBT;

    void resizedInternalHeader(Rectangle<int>& r) override;

    void buttonClicked(Button* b) override;

    void fillPopupMenuForGroup(const AudioProcessorParameterGroup  *group, PopupMenu & menu);
};