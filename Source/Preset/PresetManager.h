/*
  ==============================================================================

    PresetManager.h
    Created: 17 Apr 2021 12:08:47pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

class PresetManager :
    public BaseManager<Preset>
{
public:
    PresetManager();
    virtual ~PresetManager();
};

class RootPresetManager :
    public PresetManager,
    public Inspectable::InspectableListener,
    public EngineListener,
    public Thread
{
public:
    juce_DeclareSingleton(RootPresetManager, true);
    RootPresetManager();
    ~RootPresetManager();

    Trigger* saveCurrentTrigger;
    Trigger* loadCurrentTrigger;
    Trigger* nextPresetTrigger;
    Trigger* previousPresetTrigger;
    WeakReference<ControllableContainer> prevPreset;
    Preset* currentPreset;

    TargetParameter* presetOnFileLoad;

    StringParameter* curPresetName;

    //Transition
    
    HashMap<WeakReference<Controllable>, var> initTargetMap;



    void clear() override;

    void setCurrentPreset(Preset* p);
    void loadNextPreset(Preset* p, bool recursive);
    void loadPreviousPreset(Preset* p);
    void loadLastNestedPresetFor(Preset* p);
    void onContainerTriggerTriggered(Trigger* t) override;

    void toggleControllablePresettable(Controllable* c);
    var getControllablePresetOption(Controllable* c, String option, var defaultVal);
    void setControllablePresetOption(Controllable * c, String option, var val);
    bool isControllablePresettable(Controllable * c);

    void removeAllPresetsFor(Controllable * c);

    void inspectableDestroyed(Inspectable* i) override;

    Array<Preset*> getAllPresets(Preset * parent = nullptr);

    void fillPresetMenu(PopupMenu & menu, int indexOffset, Controllable * targetControllable);
    Preset * getPresetForMenuResult(int result);

    void run() override;
    void process(float progression, float weight);

    void endLoadFile() override;
};

