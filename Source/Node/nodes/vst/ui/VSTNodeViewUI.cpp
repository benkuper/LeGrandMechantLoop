/*
  ==============================================================================

	VSTNodeViewUI.cpp
	Created: 15 Nov 2020 11:41:16pm
	Author:  bkupe

  ==============================================================================
*/

#include "Engine/ui/VSTManagerUI.h"
#include "Node/NodeIncludes.h"

VSTNodeViewUI::VSTNodeViewUI(VSTNode* n) :
	NodeViewUI(n)
{
	editHeaderBT.reset(AssetManager::getInstance()->getEditBT());
	addAndMakeVisible(editHeaderBT.get());
	editHeaderBT->addListener(this);


	pluginUI.reset(new VSTPluginParameterUI(n->pluginParam));
	addChildComponent(pluginUI.get());
	pluginUI->setVisible(n->showPluginParam->boolValue());

	midiParamUI.reset(node->midiInterfaceParam->createTargetUI());
	addChildComponent(midiParamUI.get());
	midiParamUI->setVisible(n->showMidiDevice->boolValue());

	contentComponents.add(pluginUI.get());
	contentComponents.add(midiParamUI.get());

	rebuildMacroUIs();
}

VSTNodeViewUI::~VSTNodeViewUI()
{
}

void VSTNodeViewUI::resizedInternalHeader(Rectangle<int>& r)
{
	NodeViewUI::resizedInternalHeader(r);
	editHeaderBT->setBounds(r.removeFromRight(r.getHeight()).reduced(1));
}

void VSTNodeViewUI::resizedInternalContentNode(Rectangle<int>& r)
{
	if (pluginUI->isVisible())
	{
		pluginUI->setBounds(r.removeFromTop(20).reduced(1));
		r.removeFromTop(4);
	}

	if (midiParamUI->isVisible())
	{
		midiParamUI->setBounds(r.removeFromTop(20).reduced(1));
		r.removeFromTop(4);
	}

	for (auto& m : macroUIs)
	{
		if (!m->isVisible()) continue;
		m->setBounds(r.removeFromTop(20).reduced(1));
		r.removeFromTop(2);
	}
}

void VSTNodeViewUI::controllableFeedbackUpdateInternal(Controllable* c)
{
	NodeViewUI::controllableFeedbackUpdateInternal(c);
	if (c == node->numMacros) rebuildMacroUIs();
}

void VSTNodeViewUI::buttonClicked(Button* b)
{
	NodeViewUI::buttonClicked(b);
	if (b == editHeaderBT.get())
	{
		if (node->vst != nullptr && node->vst->hasEditor())
		{
			if (pluginEditor.get() == nullptr)
			{
				pluginEditor.reset(new PluginWindow(node));
				if (pluginEditorBounds.isEmpty()) pluginEditor->centreWithSize(pluginEditor->getWidth(), pluginEditor->getHeight());
				else pluginEditor->setBounds(pluginEditorBounds);

				pluginEditor->addPluginWindowListener(this);
				//editHeaderBT->setEnabled(false);
			}
			else
			{
				//pluginEditor->toFront(true);
			}

			pluginEditor->toFront(true);
			pluginEditor->centreAroundComponent(getTopLevelComponent(), pluginEditor->getWidth(), pluginEditor->getHeight());

		}
	}
}

void VSTNodeViewUI::viewFilterUpdated()
{
	pluginUI->setVisible(node->showPluginParam->boolValue());
	midiParamUI->setVisible(node->showMidiDevice->boolValue());
	rebuildMacroUIs();
	NodeViewUI::viewFilterUpdated();
}

void VSTNodeViewUI::rebuildMacroUIs()
{
	for (auto& m : macroUIs) contentComponents.removeAllInstancesOf(m);

	macroUIs.clear();

	if (node->showMacros->boolValue())
	{
		for (auto& c : node->macrosCC.controllables)
		{
			macroUIs.add(c->createDefaultUI());
			addAndMakeVisible(macroUIs.getLast());
			contentComponents.add(macroUIs.getLast());

		}
	}

	resized();

}

void VSTNodeViewUI::windowClosed()
{
	pluginEditorBounds = pluginEditor->getBounds();
	pluginEditor.reset();
	//editHeaderBT->setEnabled(true);
}

PluginWindow::PluginWindow(VSTNode* node) :
	DocumentWindow(node->vst->getName(), BG_COLOR.darker(.1f), DocumentWindow::closeButton, true),
	inspectable(node),
	node(node)
{
	setVSTEditor(node->vst.get());
	//const Displays::Display* d = Desktop::getInstance().getDisplays().getDisplayForRect(getScreenBounds());
	//if (d != nullptr) setCentrePosition(d->userArea.getCentre());

	setResizable(true, true);
	setDraggable(true);

	//setAlwaysOnTop(true);

	node->addAsyncVSTListener(this);
}

PluginWindow::~PluginWindow()
{
	if (!inspectable.wasObjectDeleted()) node->removeAsyncVSTListener(this);
}

void PluginWindow::resized()
{
	DocumentWindow::resized();
}

void PluginWindow::newMessage(const VSTNode::VSTEvent& e)
{
	if (e.type == e.VST_REMOVED) setVSTEditor(nullptr);
	else if (e.type == e.VST_SET) setVSTEditor(node->vst.get());
}

void PluginWindow::setVSTEditor(AudioPluginInstance* vstInstance)
{
	if (vstInstance != nullptr)
	{
		setContentOwned(vstInstance->createEditor(), true);
		setVisible(true);
        setAlwaysOnTop(true);
		toFront(true);
		if (getX() < 0 || getY() < 0)
		{
			setTopLeftPosition(50, 50);
		}
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

