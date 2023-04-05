/*
  ==============================================================================

    PresetManagerUI.h
    Created: 17 Apr 2021 12:09:12pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

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
    std::unique_ptr<TriggerUI> saveCurrentUI;
    std::unique_ptr<FloatSliderUI> loadProgressionUI;

    void resizedInternalHeader(Rectangle<int>& r) override;

    static RootPresetManagerUI* create(const String& name) { return new RootPresetManagerUI(); }
};