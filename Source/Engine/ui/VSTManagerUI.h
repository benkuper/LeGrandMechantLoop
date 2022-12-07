/*
  ==============================================================================

    VSTManagerUI.h
    Created: 24 Nov 2020 11:04:12am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "../VSTManager.h"

class VSTManagerEditor :
    public GenericControllableContainerEditor,
    public VSTManager::AsyncListener
{
public:
    VSTManagerEditor(VSTManager* vstManager, bool isRoot);
    ~VSTManagerEditor() {}

    VSTManager* vstManager;
    std::unique_ptr<TriggerButtonUI> rescanUI;
    std::unique_ptr<BoolToggleUI> vstUI;
    std::unique_ptr<BoolToggleUI> auUI;
    std::unique_ptr<BoolToggleUI> lv2UI;
    Label countLabel;

    void resizedInternalHeader(Rectangle<int>& r) override;

    void newMessage(const VSTManager::VSTManagerEvent& e) override;
};

class VSTPluginParameterUI :
    public ParameterUI,
    public Button::Listener
{
public:
    VSTPluginParameterUI(VSTPluginParameter * p);
    ~VSTPluginParameterUI();

    VSTPluginParameter* vstParam;
    TextButton bt;

    void updateButtonText();

    void resized() override;

    void showMenuAndSetVST();
    void buttonClicked(Button* b) override;

    void valueChanged(const var & val) override;
};