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
	addAndMakeVisible(pluginUI.get());

	midiParamUI.reset(node->midiInterfaceParam->createTargetUI());
	addAndMakeVisible(midiParamUI.get());

	contentComponents.add(pluginUI.get());
	contentComponents.add(midiParamUI.get());
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
	pluginUI->setBounds(r.removeFromTop(20).reduced(1));
	r.removeFromTop(4);

	midiParamUI->setBounds(r.removeFromTop(20).reduced(1));
}

void VSTNodeViewUI::controllableFeedbackUpdateInternal(Controllable* c)
{
	NodeViewUI::controllableFeedbackUpdateInternal(c);
	// if (c == node->pluginParam) updateVSTEditor();
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
				if(pluginEditorBounds.isEmpty()) pluginEditor->centreWithSize(pluginEditor->getWidth(), pluginEditor->getHeight());
				else pluginEditor->setBounds(pluginEditorBounds);

				pluginEditor->addPluginWindowListener(this);
				editHeaderBT->setEnabled(false);
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
	pluginEditorBounds = pluginEditor->getBounds();
	pluginEditor.reset();
	editHeaderBT->setEnabled(true);
}

PluginWindow::PluginWindow(VSTNode* node) :
	DocumentWindow(node->vst->getName(), BG_COLOR.darker(.1f), DocumentWindow::closeButton, true),
	inspectable(node),
	node(node)
{
	setVSTEditor(node->vst.get());
	const Displays::Display* d = Desktop::getInstance().getDisplays().getDisplayForRect(getScreenBounds());
	if (d != nullptr) setCentrePosition(d->userArea.getCentre());

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

