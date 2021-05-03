/*
  ==============================================================================

    MacroParameter.cpp
    Created: 3 May 2021 10:23:33am
    Author:  bkupe

  ==============================================================================
*/

#include "MacroParameter.h"

MacroParameter::MacroParameter() :
    BaseItem("Macro")
{
    value = addFloatParameter("Value", "Value of this macro", 0, 0, 1);
    value->isCustomizableByUser = true;
}

MacroParameter::~MacroParameter()
{
}
