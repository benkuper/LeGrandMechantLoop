/*
  ==============================================================================

    VSTManager.h
    Created: 24 Nov 2020 9:39:06am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"

class VSTManager :
    public ControllableContainer,
    public Thread
{
public:
    juce_DeclareSingleton(VSTManager, true)

    VSTManager();
    ~VSTManager();

    AudioPluginFormatManager formatManager;
    OwnedArray<PluginDescription> descriptions;
    HashMap<String, PluginDescription*> idDescriptionMap;

    Trigger* rescan;

    void updateVSTList();
    void onContainerParameterChanged(Parameter * p) override;
    void onControllableAdded(Controllable* c) override;
    void onControllableRemoved(Controllable* c) override;

    var getJSONData() override;
    void loadJSONDataInternal(var data) override;
    void afterLoadJSONDataInternal() override;

    void onContainerTriggerTriggered(Trigger*) override;

    void run() override;

    DECLARE_ASYNC_EVENT(VSTManager, VSTManager, vstManager, ENUM_LIST(PLUGINS_UPDATED))

    InspectableEditor* getEditor(bool isRoot) override;
};

class VSTPluginParameterUI;

class VSTPluginParameter :
    public StringParameter
{
public:
    VSTPluginParameter(const String& name, const String& description);
    ~VSTPluginParameter();

    PluginDescription* getPluginDescription();

    VSTPluginParameterUI* createVSTParamUI();
    ControllableUI * createDefaultUI() override;
};