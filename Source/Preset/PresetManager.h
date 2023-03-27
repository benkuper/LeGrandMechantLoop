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
    public Thread
{
public:
    juce_DeclareSingleton(RootPresetManager, true);
    RootPresetManager();
    ~RootPresetManager();

    Trigger* saveCurrentTrigger;
    Trigger* loadCurrentTrigger;
    Trigger* nextPreset;
    Trigger* previousPreset;
    Preset* currentPreset;

    StringParameter* curPresetName;

    //Transition
    
    HashMap<WeakReference<Parameter>, var> initTargetMap;



    void clear() override;

    void setCurrentPreset(Preset* p);
    void loadNextPreset(Preset* p, bool recursive);
    void loadPreviousPreset(Preset* p);
    void loadLastNestedPresetFor(Preset* p);
    void onContainerTriggerTriggered(Trigger* t) override;

    void toggleParameterPresettable(Parameter* p);
    var getParameterPresetOption(Parameter* p, String option, var defaultVal);
    void setParameterPresetOption(Parameter * p, String option, var val);
    bool isParameterPresettable(Parameter *p);

    void inspectableDestroyed(Inspectable* i) override;

    Array<Preset*> getAllPresets(Preset * parent = nullptr);

    void fillPresetMenu(PopupMenu & menu, int indexOffset, Parameter * targetParam = nullptr);
    Preset * getPresetForMenuResult(int result);

    void run() override;
    void lerpParameters(float progression, float weight);
};

