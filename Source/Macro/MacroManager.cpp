/*
  ==============================================================================

    MacroManager.cpp
    Created: 3 May 2021 10:23:17am
    Author:  bkupe

  ==============================================================================
*/

#include "MacroManager.h"

juce_ImplementSingleton(MacroManager)

MacroManager::MacroManager() :
    Manager("Macros")
{
}

MacroManager::~MacroManager()
{
}
