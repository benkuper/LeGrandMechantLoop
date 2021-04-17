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
}

PresetManager::~PresetManager()
{
}


//Root

RootPresetManager::RootPresetManager() : 
    PresetManager(),
    currentPreset(nullptr)
{
}

RootPresetManager::~RootPresetManager()
{
    setCurrentPreset(nullptr);
}

void RootPresetManager::setCurrentPreset(Preset* p)
{
    if (currentPreset == p) return;
    if (currentPreset != nullptr)
    {
        currentPreset->removeInspectableListener(this);
    }

    currentPreset = p;

    if (currentPreset != nullptr)
    {
        currentPreset->addInspectableListener(this);
    }
}

void RootPresetManager::inspectableDestroyed(Inspectable* i)
{
    if (i == currentPreset) setCurrentPreset(nullptr);
}
