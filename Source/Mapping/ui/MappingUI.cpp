/*
  ==============================================================================

    MappingUI.cpp
    Created: 3 May 2021 11:39:45am
    Author:  bkupe

  ==============================================================================
*/

#include "MappingUI.h"

MappingUI::MappingUI(Mapping * item) :
    BaseItemUI(item)
{
    updateValueUI();
    updateOutUI();
}

MappingUI::~MappingUI()
{
}

void MappingUI::resizedInternalHeader(Rectangle<int>& r)
{
    if (outUI != nullptr)
    {
        outUI->setBounds(r.removeFromRight(100));
        r.removeFromRight(2);
    }

    if (valueUI != nullptr)
    {
        valueUI->setBounds(r.removeFromRight(200));
    }
}

void MappingUI::updateOutUI()
{
    if (outUI != nullptr)
    {
        if (outUI->controllable == item->dest) return;
        removeChildComponent(outUI.get());
    }

    outUI.reset(item->dest != nullptr ? item->dest->createDefaultUI() : nullptr);

    if(outUI != nullptr)
    {
        addAndMakeVisible(outUI.get());
    }
    resized();
}

void MappingUI::updateValueUI()
{
    if (valueUI != nullptr)
    {
        removeChildComponent(valueUI.get());
    }


    if (GenericMapping* m = dynamic_cast<GenericMapping*>(item))
    {
        if (m->source != nullptr) valueUI.reset(m->source->createDefaultUI());
    }
    else if (MacroMapping* m = dynamic_cast<MacroMapping*>(item))
    {
        valueUI.reset(m->sourceParam->createDefaultUI());
    }
    else
    {
        valueUI.reset();
    }

    if (valueUI != nullptr)
    {
        addAndMakeVisible(valueUI.get());
    }
    resized();
}

void MappingUI::controllableFeedbackUpdateInternal(Controllable* c)
{
    if (c == item->destParam) updateOutUI();
    else if (GenericMapping* m = dynamic_cast<GenericMapping*>(item))
    {
        if (c == m->sourceParam) updateValueUI();
    }
}

bool MappingUI::isInterestedInDragSource(const DragAndDropTarget::SourceDetails& details)
{
    if (ControllableEditor* e = dynamic_cast<ControllableEditor*>(details.sourceComponent.get()))
    {
        return true;
    }

    return false;
}

void MappingUI::itemDropped(const DragAndDropTarget::SourceDetails& details)
{
    if (ControllableEditor* e = dynamic_cast<ControllableEditor*>(details.sourceComponent.get()))
    {
        if (GenericMapping* i = dynamic_cast<GenericMapping*>(item))
        {
			PopupMenu p;
			p.addItem(3, "Set as source");
			p.addItem(4, "Set as destination");

			if (int result = p.show())
			{
				if (result != 3) i->destParam->setValueFromTarget(e->controllable.get());
				else ((GenericMapping*)i)->sourceParam->setValueFromTarget(e->controllable.get());
			}
        }
        else
        {
            item->destParam->setValueFromTarget(e->controllable.get(), true);
        }
    }

    BaseItemUI::itemDropped(details);
}
