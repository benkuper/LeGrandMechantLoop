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
    public BaseManager<Preset>::ManagerListener,
    public Inspectable::InspectableListener
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

    void save(ControllableContainer* container, bool recursive);
    void save(Parameter* controllable = nullptr);
    void load(ControllableContainer* container, bool recursive);
    void load(Parameter* controllable = nullptr);

    void addParameterToDataMap(Parameter* p, var forceValue = var());
    void updateParameterAddress(Parameter* p);
    void removeParameterFromDataMap(Parameter* p);
    void removeAddressFromDataMap(String address);

    bool isMain(); //check if it's not an override

    void onContainerTriggerTriggered(Trigger* t) override;

    void itemAdded(Preset * p) override;

    var getJSONData() override;
    void loadJSONDataItemInternal(var data) override;

    void inspectableDestroyed(Inspectable* i) override;
    void controllableControlAddressChanged(Controllable* c) override;
    
    void childStructureChanged(ControllableContainer* cc) override;

    Array<Preset*> getPresetChain();

    String getTypeString() const override { return "Preset"; }
};