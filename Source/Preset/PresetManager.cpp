/*
  ==============================================================================

    PresetManager.cpp
    Created: 17 Apr 2021 12:08:47pm
    Author:  bkupe

  ==============================================================================
*/

#include "PresetManager.h"

juce_ImplementSingleton(RootPresetManager);

PresetManager::PresetManager() :
    BaseManager("Presets")
{
    itemDataType = "Preset";
}

PresetManager::~PresetManager()
{
}


//Root

RootPresetManager::RootPresetManager() : 
    PresetManager(),
    currentPreset(nullptr)
{
    saveCurrentTrigger = addTrigger("Save Current", "Save state to current preset");
}

RootPresetManager::~RootPresetManager()
{
    setCurrentPreset(nullptr);
}

void RootPresetManager::clear()
{
    setCurrentPreset(nullptr);
    BaseManager::clear();
}

void RootPresetManager::setCurrentPreset(Preset* p)
{
    if (currentPreset == p) return;
    if (currentPreset != nullptr)
    {
        currentPreset->removeInspectableListener(this);
        currentPreset->isCurrent->setValue(false);
    }

    currentPreset = p;

    if (currentPreset != nullptr)
    {
        currentPreset->addInspectableListener(this);
        currentPreset->isCurrent->setValue(true);
        currentPreset->load();
    }
}

void RootPresetManager::onContainerTriggerTriggered(Trigger* t)
{
    if (t == saveCurrentTrigger)
    {
        if (currentPreset != nullptr) currentPreset->save();
    }
}

void RootPresetManager::inspectableDestroyed(Inspectable* i)
{
    if (i == currentPreset) setCurrentPreset(nullptr);
}
