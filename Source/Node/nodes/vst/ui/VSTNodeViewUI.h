/*
  ==============================================================================

    VSTNodeViewUI.h
    Created: 15 Nov 2020 11:41:16pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "../VSTNode.h"
#include "../../../ui/NodeViewUI.h"

class PluginWindow :
    public DocumentWindow,
    public VSTProcessor::AsyncListener
{
public:
    PluginWindow(VSTProcessor* processor);
    ~PluginWindow();

    WeakReference<Inspectable> inspectable;
    VSTProcessor* processor;
    std::unique_ptr<AudioProcessorEditor> editor;

    void setVSTEditor(AudioPluginInstance * vstInstance);
    void userTriedToCloseWindow() override;
    void closeButtonPressed() override;
    void resized() override;

    void newMessage(const VSTProcessor::VSTEvent& e) override;

    class PluginWindowListener
    {
    public:
        virtual ~PluginWindowListener() {}
        virtual void windowClosed() {}
    };

    ListenerList<PluginWindowListener> pwListeners;
    void addPluginWindowListener(PluginWindowListener* newListener) { pwListeners.add(newListener); }
    void removePluginWindowListener(PluginWindowListener* listener) { pwListeners.remove(listener); }
};

class VSTNodeViewUI :
    public GenericNodeViewUI<VSTProcessor>,
    public PluginWindow::PluginWindowListener
{
public:
    VSTNodeViewUI(GenericNode<VSTProcessor>* n);
    ~VSTNodeViewUI();

    TextButton editBT;
    std::unique_ptr<PluginWindow> pluginEditor;
    std::unique_ptr<MIDIDeviceParameterUI> midiParamUI;

    void resizedInternalContentNode(Rectangle<int>& r) override;
    void controllableFeedbackUpdateInternal(Controllable* c) override;
    void buttonClicked(Button* b) override;

    void windowClosed() override;
};
