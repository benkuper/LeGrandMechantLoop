/*
  ==============================================================================

    MacroManagerUI.cpp
    Created: 3 May 2021 10:36:07am
    Author:  bkupe

  ==============================================================================
*/

#include "MacroManagerUI.h"

MacroManagerUI::MacroManagerUI(const String& name) :
    BaseManagerShapeShifterUI(name, MacroManager::getInstance())
{
    addExistingItems();
}

MacroManagerUI::~MacroManagerUI()
{
}
