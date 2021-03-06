/*
  ==============================================================================

    VSTManagerUI.cpp
    Created: 24 Nov 2020 11:04:12am
    Author:  bkupe

  ==============================================================================
*/

#include "VSTManagerUI.h"

VSTManagerEditor::VSTManagerEditor(VSTManager* vstManager, bool isRoot) :
    GenericControllableContainerEditor(vstManager, isRoot),
    vstManager(vstManager)
{
    rescanUI.reset(vstManager->rescan->createButtonUI());
    addAndMakeVisible(rescanUI.get());

    countLabel.setColour(countLabel.textColourId, TEXT_COLOR);
    countLabel.setFont(countLabel.getFont().withHeight(10));
    countLabel.setJustificationType(Justification::centred);
    addAndMakeVisible(countLabel);
}

void VSTManagerEditor::resizedInternalHeader(Rectangle<int>& r)
{
    GenericControllableContainerEditor::resizedInternalHeader(r);
    rescanUI->setBounds(r.removeFromRight(60).reduced(1));
    countLabel.setBounds(r.removeFromRight(60));
}

void VSTManagerEditor::newMessage(const VSTManager::VSTManagerEvent& e)
{
    countLabel.setText(String(vstManager->descriptions.size()) + " Plugins", dontSendNotification);
}


VSTPluginParameterUI::VSTPluginParameterUI(VSTPluginParameter* p) :
    ParameterUI(p),
    vstParam(p)
{
    bt.addListener(this);
    addAndMakeVisible(bt);
    updateButtonText();
}

VSTPluginParameterUI::~VSTPluginParameterUI()
{
}

void VSTPluginParameterUI::updateButtonText()
{
    PluginDescription* d = vstParam->getPluginDescription();
    bt.setButtonText(d != nullptr ? d->descriptiveName : "Select a VST");
}

void VSTPluginParameterUI::resized()
{
    bt.setBounds(getLocalBounds());
}

void VSTPluginParameterUI::showMenuAndSetVST()
{
    PopupMenu p;
    OwnedArray<PopupMenu> subMenus;
    HashMap<String, PopupMenu*> subMenuMap;

    int index = 1;
    for (auto& d : VSTManager::getInstance()->descriptions)
    {
        String subM = d->category;
        if (!subMenuMap.contains(subM))
        {
            PopupMenu* sp = new PopupMenu();
            subMenus.add(sp);
            subMenuMap.set(subM, sp);
        }

        subMenuMap[subM]->addItem(index++, d->descriptiveName);
    }

    HashMap<String, PopupMenu*>::Iterator it(subMenuMap);
    while (it.next()) p.addSubMenu(it.getKey(), *it.getValue());

    if (int result = p.show())
    {
        if (PluginDescription* d = VSTManager::getInstance()->descriptions[result - 1])
        {
            parameter->setValue(d->fileOrIdentifier);
        }
    }
}

void VSTPluginParameterUI::buttonClicked(Button* b)
{
    showMenuAndSetVST();
}

void VSTPluginParameterUI::valueChanged(const var& val)
{
    updateButtonText();
}
