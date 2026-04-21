/*
  ==============================================================================

    PresetUI.h
    Created: 17 Apr 2021 3:30:09pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

class PresetManagerUI;

class PresetUI :
    public ItemUI<Preset>,
    public ManagerUI<PresetManager, Preset, PresetUI>::ManagerUIListener
{
public:
    PresetUI(Preset* p);
    ~PresetUI();

    std::unique_ptr<PresetManagerUI> pmui;
    std::unique_ptr<ImageButton> addBT;
    //std::unique_ptr<ColorParameterUI> colorUI;
    std::unique_ptr<TriggerButtonUI> loadUI;

    std::unique_ptr<ControllableUI> previewLinkedUI;
    std::unique_ptr<Controllable> previewLinked;

    void paintOverChildren(Graphics& g) override;

    void mouseEnter(const MouseEvent& e) override;
    void mouseExit(const MouseEvent& e) override;
    void mouseDown(const MouseEvent& e) override;
    void addContextMenuItems(PopupMenu& p) override;
    void handleContextMenuResult(int result) override;

    void resizedInternalHeader(Rectangle<int>& r) override;
    void resizedInternalContent(Rectangle<int>& r) override;

    void itemUIAdded(PresetUI* ui) override;
    void itemUIRemoved(PresetUI* ui) override;

    void updateManagerBounds();

    void childBoundsChanged(Component* c) override;

    void controllableFeedbackUpdateInternal(Controllable* c) override;

    void buttonClicked(Button* b) override;

    void setPreviewUI(Controllable* c);

    void newMessage(const Inspectable::InspectableEvent& e) override;
};


class PresetValueEditor :
    public ControllableEditor,
    public ComboBox::Listener,
    public Parameter::AsyncListener
{
public:
    PresetValueEditor(Preset* p, Controllable* c, Controllable* sourceControllable);
    ~PresetValueEditor();

    Preset* preset;
    Controllable* sourceControllable;
    TextButton saveBT;

    ComboBox transitionMode;
    FloatParameter transitionPercent;
    std::unique_ptr<FloatSliderUI> transitionPercentUI;

    void resizedInternal(Rectangle<int> &r) override;

    void comboBoxChanged(ComboBox* cb) override;
    void buttonClicked(Button* b) override;

    void newMessage(const Parameter::ParameterEvent& e) override;
};

class PresetEditor :
    public BaseItemEditor,
    public Parameter::AsyncListener,
    public Controllable::AsyncListener
{
public:
    PresetEditor(Preset* preset, bool isRoot);
    ~PresetEditor();

    Preset* preset;

    HashMap<Controllable*, Controllable*> controllableMap;
    ControllableContainer valuesCC;
    HashMap<Controllable*, Controllable*> ignoreMap;
    ControllableContainer ignoreCC;

    void resetAndBuild() override;
    void buildValuesCC();

    void newMessage(const ContainerAsyncEvent& e) override;

    void newMessage(const Parameter::ParameterEvent& e) override;
    void newMessage(const Controllable::ControllableEvent& e) override;

    void resizedInternalContent(Rectangle<int>& r) override;
};
