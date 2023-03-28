/*
  ==============================================================================

    Preset.h
    Created: 17 Apr 2021 12:08:52pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

class PresetManager;

class Preset :
    public BaseItem,
    public BaseManager<Preset>::ManagerListener
{
public:
    Preset(var params = var());
    ~Preset();

    BoolParameter* isCurrent;
    
    Trigger* saveTrigger;
    Trigger* loadTrigger;

    BoolParameter* skipInPrevNext;
    BoolParameter* noParentOnNearbyLoad;

    FloatParameter* transitionTime;
    Automation transition;
    enum TransitionMode { INTERPOLATE, AT_START, AT_END, DEFAULT };
    EnumParameter* directTransitionMode;

    //overriding
    HashMap<WeakReference<Controllable>, var> dataMap;
    HashMap<WeakReference<Controllable>, String> controllableGhostAddressMap;
    Array<String> lostParamAddresses;
    HashMap<String, var> addressMap;
    Array<String> overridenControllables;

    std::unique_ptr<PresetManager> subPresets;

    ControllableContainer linkedPresetsCC;

    void clearItem() override;

    var getPresetValues(bool includeParents = true);
    var getPresetValueForParameter(Parameter* c, bool includeParents = true);

    void saveContainer(ControllableContainer* container, bool recursive);
    void save(Controllable* controllable = nullptr, bool saveAllPresettables = false, bool noCheck = false);
    void load(ControllableContainer* container, bool recursive);
    void load(Controllable * controllable, bool recursive);
    void load(bool recursive = false);

    void addControllableToDataMap(Controllable * c, var forceValue = var());
    void updateControllableAddress(Controllable * c);
    void removeControllableFromDataMap(Controllable * c);
    void removeAddressFromDataMap(String address);

    bool isMain(); //check if it's not an override
    bool hasPresetControllable(Controllable * c);

    void onContainerTriggerTriggered(Trigger* t) override;

    void itemAdded(Preset * p) override;

    void controllableAdded(Controllable* c); //for linked presets


    var getJSONData() override;
    void loadJSONDataItemInternal(var data) override;

    void controllableControlAddressChanged(Controllable* c) override;
    
    void childStructureChanged(ControllableContainer* cc) override;

    Array<Preset*> getPresetChain();

    InspectableEditor* getEditorInternal(bool isRoot, Array<Inspectable*> controllables = {}) override;

    DECLARE_TYPE("Preset");
};