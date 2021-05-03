/*
  ==============================================================================

    MacroManager.h
    Created: 3 May 2021 10:23:17am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "MacroParameter.h"

class MacroManager :
    public BaseManager<MacroParameter>
{
public:
    juce_DeclareSingleton(MacroManager, true);

    MacroManager();
    ~MacroManager();
};