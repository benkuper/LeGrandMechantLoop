/*
  ==============================================================================

    PresetUI.h
    Created: 17 Apr 2021 3:30:09pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "../Preset.h"

class PresetManagerUI;

class PresetUI :
    public BaseItemUI<Preset>,
    public BaseManagerUI<PresetManager,Preset,PresetUI>::ManagerUIListener
{
public:
    PresetUI(Preset* p);
    ~PresetUI();

    std::unique_ptr<PresetManagerUI> pmui;
    std::unique_ptr<ImageButton> addBT;
    std::unique_ptr<ColorParameterUI> colorUI;

    void resizedInternalHeader(Rectangle<int>& r) override;
    void resizedInternalContent(Rectangle<int> &r) override;

    void itemUIAdded(PresetUI* ui) override;
    void itemUIRemoved(PresetUI* ui) override;

    void updateManagerBounds();

    void childBoundsChanged(Component* c) override;

    void controllableFeedbackUpdateInternal(Controllable * c) override;

    void buttonClicked(Button * b) override;
};