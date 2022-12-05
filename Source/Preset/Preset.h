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

    ColorParameter* color;
    Trigger* saveTrigger;
    Trigger* loadTrigger;
    BoolParameter* isCurrent;

    //overriding
    HashMap<WeakReference<Parameter>, var> dataMap;
    HashMap<WeakReference<Parameter>, String> paramGhostAddressMap;
    Array<String> lostParamAddresses;
    HashMap<String, var> addressMap;
    Array<String> overridenControllables;

    std::unique_ptr<PresetManager> subPresets;

    void clearItem() override;

    var getPresetValues(bool includeParents = true);
    var getPresetValueForParameter(Parameter* c);

    void saveContainer(ControllableContainer* container, bool recursive);
    void save(Parameter* controllable = nullptr, bool saveAllPresettables = false, bool noCheck = false);
    void load(ControllableContainer* container, bool recursive);
    void load(Parameter* controllable = nullptr);

    void addParameterToDataMap(Parameter* p, var forceValue = var());
    void updateParameterAddress(Parameter* p);
    void removeParameterFromDataMap(Parameter* p);
    void removeAddressFromDataMap(String address);

    bool isMain(); //check if it's not an override
    bool hasPresetParam(Parameter* p);

    void onContainerTriggerTriggered(Trigger* t) override;

    void itemAdded(Preset * p) override;

    var getJSONData() override;
    void loadJSONDataItemInternal(var data) override;

    void controllableControlAddressChanged(Controllable* c) override;
    
    void childStructureChanged(ControllableContainer* cc) override;

    Array<Preset*> getPresetChain();

    InspectableEditor* getEditorInternal(bool isRoot, Array<Inspectable*> controllables = {}) override;

    String getTypeString() const override { return "Preset"; }
};