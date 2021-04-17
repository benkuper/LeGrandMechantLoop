/*
  ==============================================================================

    PresetManager.h
    Created: 17 Apr 2021 12:08:47pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once
#include "Preset.h"

class PresetManager :
    public BaseManager<Preset>
{
public:
    PresetManager();
    virtual ~PresetManager();
};

class RootPresetManager :
    public PresetManager,
    public Inspectable::InspectableListener
{
public:
    juce_DeclareSingleton(RootPresetManager, true);
    RootPresetManager();
    ~RootPresetManager();

    Preset* currentPreset;

    void setCurrentPreset(Preset* p);

    void inspectableDestroyed(Inspectable* i) override;
};

