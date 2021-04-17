/*
  ==============================================================================

	PresetUI.cpp
	Created: 17 Apr 2021 3:30:09pm
	Author:  bkupe

  ==============================================================================
*/

#include "PresetUI.h"
#include "PresetManagerUI.h"

PresetUI::PresetUI(Preset* p) :
	BaseItemUI(p, NONE, true)
{
	addBT.reset(AssetManager::getInstance()->getAddBT());
	addAndMakeVisible(addBT.get());
	addBT->addListener(this);

	colorUI.reset(item->color->createColorParamUI());
	addAndMakeVisible(colorUI.get());

	pmui.reset(new PresetManagerUI(p->subPresets.get()));
	contentComponents.add(pmui.get());
	addAndMakeVisible(pmui.get());
	pmui->addManagerUIListener(this);
	pmui->setShowAddButton(false);
	pmui->headerSize = 0;
	pmui->setVisible(pmui->manager->items.size() > 0);

	pmui->resized();
	updateManagerBounds();

	bgColor = item->color->getColor();
}

PresetUI::~PresetUI()
{
}

void PresetUI::resizedInternalHeader(Rectangle<int>& r)
{
	addBT->setBounds(r.removeFromRight(r.getHeight()));
	colorUI->setBounds(r.removeFromRight(r.getHeight()).reduced(1));
}

void PresetUI::resizedInternalContent(Rectangle<int>& r)
{
	if (pmui->isVisible())
	{
		r.setHeight(jmax(1, pmui->getHeight()));
		pmui->setBounds(r);
	}

}

void PresetUI::itemUIAdded(PresetUI* ui)
{
	pmui->setVisible(pmui->manager->items.size() > 0);
	resized();
}

void PresetUI::itemUIRemoved(PresetUI* ui)
{
	pmui->setVisible(pmui->manager->items.size() > 0);
	resized();
}

void PresetUI::updateManagerBounds()
{
	if (inspectable.wasObjectDeleted() || item->miniMode->boolValue()) return;
	int th = getHeightWithoutContent() + pmui->getHeight();
	if (th != getHeight())
	{
		item->listUISize->setValue(th);
		resized();
	}
}

void PresetUI::childBoundsChanged(Component* c)
{
	updateManagerBounds();
}

void PresetUI::controllableFeedbackUpdateInternal(Controllable* c)
{
	BaseItemUI::controllableFeedbackUpdateInternal(c);

	if (c == item->color)
	{
		bgColor = item->color->getColor();
		repaint();
	}
}

void PresetUI::buttonClicked(Button* b)
{
	BaseItemUI::buttonClicked(b);
	if (b == addBT.get())
	{
		pmui->manager->addItem();
	}
}
