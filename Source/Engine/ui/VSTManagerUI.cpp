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

	vstUI.reset(vstManager->scanVST->createToggle());
	vstUI->customLabel = "VST";
	addAndMakeVisible(vstUI.get());

	if (vstManager->scanAU != nullptr)
	{
		auUI.reset(vstManager->scanAU->createToggle());
		auUI->customLabel = "AU";
		addAndMakeVisible(auUI.get());
	}

	if (vstManager->scanLV2 != nullptr)
	{
		lv2UI.reset(vstManager->scanLV2->createToggle());
		lv2UI->customLabel = "LV2";
		addAndMakeVisible(lv2UI.get());
	}

	countLabel.setColour(countLabel.textColourId, TEXT_COLOR);
	countLabel.setFont(countLabel.getFont().withHeight(10));
	countLabel.setJustificationType(Justification::centred);
	addAndMakeVisible(countLabel);
}

void VSTManagerEditor::resizedInternalHeader(Rectangle<int>& r)
{
	GenericControllableContainerEditor::resizedInternalHeader(r);
	rescanUI->setBounds(r.removeFromRight(60).reduced(1));
	vstUI->setBounds(r.removeFromRight(50).reduced(2));
	if (auUI != nullptr) auUI->setBounds(r.removeFromRight(50).reduced(2));
	if (lv2UI != nullptr) lv2UI->setBounds(r.removeFromRight(50).reduced(2));
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
	bt.setButtonText(d != nullptr ? (d->descriptiveName + " (" + d->pluginFormatName + ")") : "Select a VST");
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
		String subM = d->manufacturerName;
		if (!subMenuMap.contains(subM))
		{
			PopupMenu* sp = new PopupMenu();
			subMenus.add(sp);
			subMenuMap.set(subM, sp);
		}

		subMenuMap[subM]->addItem(index++, d->descriptiveName + " (" + d->pluginFormatName + ")");
	}

	HashMap<String, PopupMenu*>::Iterator it(subMenuMap);
	while (it.next()) p.addSubMenu(it.getKey(), *it.getValue());

	p.showMenuAsync(PopupMenu::Options().withDeletionCheck(*this), [=](int result)
		{
			if (PluginDescription* d = VSTManager::getInstance()->descriptions[result - 1])
			{
				String pid = d->manufacturerName + "/" + d->name;
				parameter->setValue(pid);
			}
		});
}

void VSTPluginParameterUI::buttonClicked(Button* b)
{
	showMenuAndSetVST();
}

void VSTPluginParameterUI::valueChanged(const var& val)
{
	updateButtonText();
}
