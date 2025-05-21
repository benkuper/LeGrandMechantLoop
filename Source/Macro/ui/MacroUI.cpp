/*
  ==============================================================================

    MacroUI.cpp
    Created: 3 May 2021 10:36:03am
    Author:  bkupe

  ==============================================================================
*/

#include "MacroUI.h"

MacroUI::MacroUI(MacroParameter* item) :
    ItemUI(item)
{
    valueUI.reset(item->value->createSlider());
    addAndMakeVisible(valueUI.get());
}

MacroUI::~MacroUI()
{
}

void MacroUI::resizedInternalHeader(Rectangle<int>& r)
{
    valueUI->setBounds(r.removeFromRight(200));
}
