/*
  ==============================================================================

    Preset.h
    Created: 17 Apr 2021 12:08:52pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"

class PresetManager;

class Preset :
    public BaseItem,
    public BaseManager<Preset>::ManagerListener
{
public:
    Preset(var params = var());
    ~Preset();

    ColorParameter* color;

    //overriding
    HashMap<Controllable *, var> dataMap;
    HashMap<String, var> addressMap;

    Array<String> overridenControllables;

    std::unique_ptr<PresetManager> subPresets;

    var getPresetData();
    var getPresetDataForControllable(Controllable* c);

    void save(ControllableContainer* container, bool recursive);
    void save(Controllable* controllable = nullptr);
    void load(ControllableContainer* container, bool recursive);
    void load(Controllable * controllable = nullptr);

    bool isMain(); //check if it's not an override

    void itemAdded(Preset * p) override;

    Array<Preset*> getPresetChain();

    String getTypeString() const override { return "Preset"; }
};