/*
  ==============================================================================

    PresetManagerUI.h
    Created: 17 Apr 2021 12:09:12pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "../PresetManager.h"
#include "PresetUI.h"

class PresetManagerUI :
    public BaseManagerUI<PresetManager, Preset, PresetUI>
{
public:
    PresetManagerUI(PresetManager* manager);
    virtual ~PresetManagerUI();

    void addItemUIInternal(PresetUI* ui) override;
    void removeItemUIInternal(PresetUI* ui) override;
};

class RootPresetManagerUI :
    public BaseManagerShapeShifterUI<PresetManager, Preset, PresetUI>
{
public:
    RootPresetManagerUI();
    ~RootPresetManagerUI();

    static RootPresetManagerUI* create(const String& name) { return new RootPresetManagerUI(); }
};