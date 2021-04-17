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
    addExistingItems();
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

    saveCurrentUI.reset(RootPresetManager::getInstance()->saveCurrentTrigger->createButtonUI());
    addAndMakeVisible(saveCurrentUI.get());
    addExistingItems();
}

RootPresetManagerUI::~RootPresetManagerUI()
{
}

void RootPresetManagerUI::resizedInternalHeader(Rectangle<int>& r)
{
    BaseManagerShapeShifterUI::resizedInternalHeader(r);
    saveCurrentUI->setBounds(r.removeFromRight(80).reduced(2));
}
