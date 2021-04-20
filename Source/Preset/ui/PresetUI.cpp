/*
  ==============================================================================

	PresetUI.cpp
	Created: 17 Apr 2021 3:30:09pm
	Author:  bkupe

  ==============================================================================
*/

#include "PresetUI.h"
#include "PresetManagerUI.h"
#include "Node/NodeManager.h"

PresetUI::PresetUI(Preset* p) :
	BaseItemUI(p, NONE, true)
{
	addBT.reset(AssetManager::getInstance()->getAddBT());
	addAndMakeVisible(addBT.get());
	addBT->addListener(this);

	colorUI.reset(item->color->createColorParamUI());
	addAndMakeVisible(colorUI.get());

	loadUI.reset(item->loadTrigger->createButtonUI());
	addAndMakeVisible(loadUI.get());

	pmui.reset(new PresetManagerUI(p->subPresets.get()));
	contentComponents.add(pmui.get());
	addAndMakeVisible(pmui.get());
	pmui->addManagerUIListener(this);
	pmui->setShowAddButton(false);
	pmui->headerSize = 0;
	pmui->setVisible(pmui->manager->items.size() > 0);

	pmui->resized();
	updateManagerBounds();

	highlightLinkedInspectablesOnOver = false;

	setDisableDefaultMouseEvents(true);
	bgColor = item->color->getColor();
}

PresetUI::~PresetUI()
{
}

void PresetUI::paintOverChildren(Graphics& g)
{
	BaseItemUI::paintOverChildren(g);
	if (item->isCurrent->boolValue())
	{
		g.setColour(GREEN_COLOR);
		g.drawRoundedRectangle(getLocalBounds().reduced(2).toFloat(), 2, 1);
	}
}

void PresetUI::mouseEnter(const MouseEvent& e)
{
	BaseItemUI::mouseEnter(e);
	if (e.eventComponent == this) inspectable->highlightLinkedInspectables(true);
}

void PresetUI::mouseExit(const MouseEvent& e)
{
	BaseItemUI::mouseExit(e);
	if (e.eventComponent == this) inspectable->highlightLinkedInspectables(false);
}

void PresetUI::mouseDown(const MouseEvent& e)
{
	BaseItemUI::mouseDown(e);
	if (e.eventComponent == this && e.mods.isLeftButtonDown() && e.mods.isAltDown()) item->loadTrigger->trigger();
}

void PresetUI::addContextMenuItems(PopupMenu& p)
{
	p.addItem(100, "Load");
	p.addItem(101, "Save");
}

void PresetUI::handleContextMenuResult(int result)
{
	if (result == 100) item->loadTrigger->trigger();
	else if (result == 101) item->saveTrigger->trigger();
}

void PresetUI::resizedInternalHeader(Rectangle<int>& r)
{
	addBT->setBounds(r.removeFromRight(r.getHeight()));
	colorUI->setBounds(r.removeFromRight(r.getHeight()).reduced(1));
	loadUI->setBounds(r.removeFromRight(80));
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
	else if (c == item->isCurrent)
	{
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


PresetEditor::PresetEditor(Preset* preset, bool isRoot) :
	BaseItemEditor(preset, isRoot),
	preset(preset),
	valuesCC("Values")
{
	HashMap<WeakReference<Parameter>, var>::Iterator it(preset->dataMap);
	while (it.next())
	{
		Parameter* op = it.getKey();
		if(op == nullptr) continue;
		Parameter* p = ControllableFactory::createParameterFrom(it.getKey().get(), false, false);
		p->setValue(it.getValue());
		String s = op->niceName;
		ControllableContainer* pc = op->parentContainer;
		while(pc != nullptr && pc != RootNodeManager::getInstance())
		{
			if (dynamic_cast<NodeManager*>(pc))
			{
				pc = pc->parentContainer;
				continue;
			}

			s = pc->niceName + ">" + s;
			pc = pc->parentContainer;
		}

		p->setNiceName(s);
		p->addAsyncParameterListener(this);
		paramMap.set(p, op);
		valuesCC.addParameter(p);
	}

	valuesCC.sortControllables();

	resetAndBuild();
}

PresetEditor::~PresetEditor()
{
}

void PresetEditor::resetAndBuild()
{
	BaseItemEditor::resetAndBuild();
	GenericControllableContainerEditor * e = (GenericControllableContainerEditor *)addEditorUI(&valuesCC, true);
	for (auto& ce : e->childEditors)
	{
		if (ControllableEditor* cce = dynamic_cast<ControllableEditor*>(ce)) cce->minLabelWidth = 250;
	}
}

void PresetEditor::buildValuesCC()
{

}

void PresetEditor::newMessage(const Parameter::ParameterEvent& e)
{
	if (e.type == Parameter::ParameterEvent::VALUE_CHANGED)
	{
		preset->addParameterToDataMap(paramMap[e.parameter], e.parameter->value);
	}
}

void PresetEditor::resizedInternalContent(Rectangle<int>& r)
{
	BaseItemEditor::resizedInternalContent(r);
}