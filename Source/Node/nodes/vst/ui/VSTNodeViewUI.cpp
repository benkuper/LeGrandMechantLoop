/*
  ==============================================================================

    VSTNodeViewUI.cpp
    Created: 15 Nov 2020 11:41:16pm
    Author:  bkupe

  ==============================================================================
*/

#include "VSTNodeViewUI.h"
#include "Common/MIDI/ui/MIDIDeviceParameterUI.h"

VSTNodeViewUI::VSTNodeViewUI(GenericNode<VSTProcessor>* n) :
    GenericNodeViewUI(n),
    editBT("Edit VST")
{
    editBT.addListener(this);
    addAndMakeVisible(&editBT);

    midiParamUI.reset(processor->midiParam->createMIDIParameterUI());
    addAndMakeVisible(midiParamUI.get());
}

VSTNodeViewUI::~VSTNodeViewUI()
{
}


void VSTNodeViewUI::resizedHeader(Rectangle<int>& r)
{
    GenericNodeViewUI::resizedHeader(r);
    editBT.setBounds(r.removeFromRight(100).reduced(1));
    midiParamUI->setBounds(r.reduced(1));
}

void VSTNodeViewUI::controllableFeedbackUpdateInternal(Controllable* c)
{
    GenericNodeViewUI::controllableFeedbackUpdateInternal(c);
   // if (c == processor->pluginParam) updateVSTEditor();
}

void VSTNodeViewUI::buttonClicked(Button* b)
{
    GenericNodeViewUI::buttonClicked(b);
    if (b == &editBT)
    {
        if (processor->vst != nullptr && processor->vst->hasEditor())
        {
            if (pluginEditor == nullptr)
            {
                pluginEditor.reset(new PluginWindow(processor));
                pluginEditor->addPluginWindowListener(this);
            }
            else
            {
                //pluginEditor->toFront(true);
            }
        }
    }
}

void VSTNodeViewUI::windowClosed()
{
    pluginEditor.reset();
}

PluginWindow::PluginWindow(VSTProcessor* processor) :
    DocumentWindow(processor->vst->getName(), BG_COLOR.darker(.1f), DocumentWindow::closeButton, true),
    inspectable(processor),
    processor(processor)
{
    setVSTEditor(processor->vst.get());
    const Displays::Display* d = Desktop::getInstance().getDisplays().getDisplayForRect(getScreenBounds());
    if(d != nullptr) setCentrePosition(d->userArea.getCentre());

    setResizable(true, true);
    setDraggable(true);

    setAlwaysOnTop(true);

    processor->addAsyncVSTListener(this);
}

PluginWindow::~PluginWindow()
{
    if(!inspectable.wasObjectDeleted()) processor->removeAsyncVSTListener(this);
}

void PluginWindow::resized()
{
    DocumentWindow::resized();
}

void PluginWindow::newMessage(const VSTProcessor::VSTEvent& e)
{
    if (e.type == e.VST_REMOVED) setVSTEditor(nullptr);
    else if (e.type == e.VST_SET) setVSTEditor(processor->vst.get());
}

void PluginWindow::setVSTEditor(AudioPluginInstance* vstInstance)
{
    if (vstInstance != nullptr)
    {
        setContentOwned(vstInstance->createEditor(), true);
        setVisible(true);
        toFront(true);
    }
    else
    {
        setContentOwned(nullptr, false);
    }
}

void PluginWindow::userTriedToCloseWindow()
{
    setContentOwned(nullptr, false);
    removeFromDesktop();
    pwListeners.call(&PluginWindowListener::windowClosed);
}

void PluginWindow::closeButtonPressed()
{
    setContentOwned(nullptr, false);
    removeFromDesktop();
    pwListeners.call(&PluginWindowListener::windowClosed);
}

