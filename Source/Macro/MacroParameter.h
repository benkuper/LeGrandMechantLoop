/*
  ==============================================================================

    MacroParameter.h
    Created: 3 May 2021 10:23:33am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"

class MacroParameter :
    public BaseItem
{
public:
    MacroParameter();
    ~MacroParameter();

    FloatParameter* value;
    
    String getTypeString() const override { return "Macro"; }
};