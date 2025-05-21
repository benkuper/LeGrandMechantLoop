/*
  ==============================================================================

    MacroUI.h
    Created: 3 May 2021 10:36:03am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "../MacroParameter.h"

class MacroUI :
    public ItemUI<MacroParameter>
{
public:
    MacroUI(MacroParameter* item);
    ~MacroUI();

    std::unique_ptr<FloatSliderUI> valueUI;

    void resizedInternalHeader(Rectangle<int>& r) override;
};