/*
  ==============================================================================

    PresetManagerUI.cpp
    Created: 17 Apr 2021 12:09:12pm
    Author:  bkupe

  ==============================================================================
*/

#include "PresetManagerUI.h"

PresetManagerUI::PresetManagerUI(PresetManager* manager) :
    BaseManagerUI(manager->niceName, manager, false)
{
    //animateItemOnAdd = false;
}

PresetManagerUI::~PresetManagerUI()
{
}

void PresetManagerUI::addItemUIInternal(PresetUI* ui)
{
    resized();
}

void PresetManagerUI::removeItemUIInternal(PresetUI* ui)
{
    resized();
}

RootPresetManagerUI::RootPresetManagerUI() :
    BaseManagerShapeShifterUI(RootPresetManager::getInstance()->niceName, RootPresetManager::getInstance())
{
}

RootPresetManagerUI::~RootPresetManagerUI()
{
}
