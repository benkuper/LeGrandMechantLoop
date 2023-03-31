/*
  ==============================================================================

	PresetUI.cpp
	Created: 17 Apr 2021 3:30:09pm
	Author:  bkupe

  ==============================================================================
*/

#include "Node/NodeIncludes.h"
#include "Preset/PresetIncludes.h"

PresetUI::PresetUI(Preset* p) :
	BaseItemUI(p, NONE, true)
{
	addBT.reset(AssetManager::getInstance()->getAddBT());
	addAndMakeVisible(addBT.get());
	addBT->addListener(this);

	//colorUI.reset(item->itemColor->createColorParamUI());
	//addAndMakeVisible(colorUI.get());

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
	bgColor = item->itemColor->getColor();
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
	if (e.eventComponent == this)
	{
		BaseItemUI::mouseDown(e);
		if (e.mods.isLeftButtonDown() && e.mods.isAltDown()) item->loadTrigger->trigger();
	}
}

void PresetUI::addContextMenuItems(PopupMenu& p)
{
	p.addItem(100, "Load");
	p.addItem(101, "Save");
	p.addItem(102, "Add all presettables");
}

void PresetUI::handleContextMenuResult(int result)
{
	if (result == 100) item->loadTrigger->trigger();
	else if (result == 101) item->saveTrigger->trigger();
	else if (result == 102) item->save(nullptr, true);
}

void PresetUI::resizedInternalHeader(Rectangle<int>& r)
{
	addBT->setBounds(r.removeFromRight(r.getHeight()));
	//colorUI->setBounds(r.removeFromRight(r.getHeight()).reduced(1));
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

	if (c == item->itemColor)
	{
		bgColor = item->itemColor->getColor();
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
	valuesCC("Values"),
	ignoreCC("Ignore Parameters")
{
	buildValuesCC();
}

PresetEditor::~PresetEditor()
{
}

void PresetEditor::resetAndBuild()
{
	BaseItemEditor::resetAndBuild();
	GenericControllableContainerEditor* ve = (GenericControllableContainerEditor*)addEditorUI(&valuesCC, true);
	for (auto& ce : ve->childEditors)
	{
		if (ControllableEditor* cce = dynamic_cast<ControllableEditor*>(ce)) cce->minLabelWidth = 250;
	}

	GenericControllableContainerEditor* ie = (GenericControllableContainerEditor*)addEditorUI(&ignoreCC, true);
	for (auto& ce : ie->childEditors)
	{
		if (ControllableEditor* cce = dynamic_cast<ControllableEditor*>(ce)) cce->minLabelWidth = 250;
	}
}

void PresetEditor::buildValuesCC()
{
	valuesCC.clear();
	controllableMap.clear();

	HashMap<WeakReference<Controllable>, var>::Iterator it(preset->dataMap);
	while (it.next())
	{
		Controllable* oc = it.getKey();
		if (oc == nullptr) continue;

		Controllable* c = nullptr;
		if (oc->type == Controllable::TRIGGER) c = new Trigger(oc->niceName, "Preset for this trigger");
		else c = ControllableFactory::createParameterFrom(oc, false, false);

		c->showWarningInUI = true;
		if (!RootPresetManager::getInstance()->isControllablePresettable(oc)) c->setWarningMessage("This parameter is not presettable");

		c->isRemovableByUser = true;
		if (c->type != Controllable::TRIGGER) ((Parameter*)c)->setValue(it.getValue());
		String s = oc->niceName;
		ControllableContainer* pc = oc->parentContainer;
		while (pc != nullptr && pc != RootNodeManager::getInstance())
		{
			if (dynamic_cast<NodeManager*>(pc))
			{
				pc = pc->parentContainer;
				continue;
			}

			s = pc->niceName + ">" + s;
			pc = pc->parentContainer;
		}

		c->setNiceName(s);
		if (c->type != Controllable::TRIGGER) ((Parameter*)c)->addAsyncParameterListener(this);
		controllableMap.set(c, oc);
		valuesCC.addControllable(c);
	}

	valuesCC.sortControllables();
	valuesCC.addAsyncContainerListener(this);



	//IGNORES
	ignoreCC.clear();
	ignoreMap.clear();

	for (auto& oc : preset->ignoredControllables)
	{
		if (oc == nullptr) continue;

		Controllable* c = nullptr;
		if (oc->type == Controllable::TRIGGER) c = new Trigger(oc->niceName, "Preset for this trigger");
		else c = ControllableFactory::createParameterFrom(oc, true, true);

		c->showWarningInUI = true;
		if (!RootPresetManager::getInstance()->isControllablePresettable(oc)) c->setWarningMessage("This parameter is not presettable");

		c->isRemovableByUser = true;
		String s = oc->niceName;
		ControllableContainer* pc = oc->parentContainer;
		while (pc != nullptr && pc != RootNodeManager::getInstance())
		{
			if (dynamic_cast<NodeManager*>(pc))
			{
				pc = pc->parentContainer;
				continue;
			}

			s = pc->niceName + ">" + s;
			pc = pc->parentContainer;
		}

		c->setNiceName(s);
		ignoreMap.set(c, oc);
		ignoreCC.addControllable(c);
	}

	ignoreCC.sortControllables();
	ignoreCC.addAsyncContainerListener(this);

	resetAndBuild();
}

void PresetEditor::newMessage(const ContainerAsyncEvent& e)
{
	if (e.source == &valuesCC || e.source == &ignoreCC)
	{
		if (e.type == ContainerAsyncEvent::ControllableRemoved)
		{
			if (e.targetControllable != nullptr && !e.targetControllable.wasObjectDeleted())
			{
				Controllable* c = e.targetControllable.get();
				if (e.source == &valuesCC)
				{
					preset->removeControllableFromDataMap(controllableMap[c]);
				}

				if (e.source == &ignoreCC)
				{
					preset->ignoredControllables.removeAllInstancesOf(ignoreMap[c]);
				}

				buildValuesCC();
			}
		}

	}
	else
	{
		BaseItemEditor::newMessage(e);
	}
}

void PresetEditor::newMessage(const Parameter::ParameterEvent& e)
{
	if (e.type == Parameter::ParameterEvent::VALUE_CHANGED)
	{
		preset->addControllableToDataMap(controllableMap[e.parameter], e.parameter->value);
	}
}

void PresetEditor::resizedInternalContent(Rectangle<int>& r)
{
	BaseItemEditor::resizedInternalContent(r);
}