/*
  ==============================================================================

    MacroManagerUI.h
    Created: 3 May 2021 10:36:07am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "MacroUI.h"
#include "../MacroManager.h"

class MacroManagerUI :
    public BaseManagerShapeShifterUI<MacroManager, MacroParameter, MacroUI>
{
public:
    MacroManagerUI(const String& name);
    ~MacroManagerUI();

    static MacroManagerUI* create(const String& name) { return new MacroManagerUI(name); }
};