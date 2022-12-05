/*
  ==============================================================================

	MappingManagerUI.cpp
	Created: 3 May 2021 11:39:41am
	Author:  bkupe

  ==============================================================================
*/

#include "MappingManagerUI.h"

MappingManagerUI::MappingManagerUI(const String& name) :
	BaseManagerShapeShifterUI(name, MappingManager::getInstance())
{
	addExistingItems();
}

MappingManagerUI::~MappingManagerUI()
{
}

bool MappingManagerUI::isInterestedInDragSource(const DragAndDropTarget::SourceDetails& details)
{
	if (ControllableEditor* e = dynamic_cast<ControllableEditor*>(details.sourceComponent.get()))
	{
		return true;
	}

	return BaseManagerShapeShifterUI::isInterestedInDragSource(details);
}

void MappingManagerUI::itemDropped(const DragAndDropTarget::SourceDetails& details)
{
	if (ControllableEditor* e = dynamic_cast<ControllableEditor*>(details.sourceComponent.get()))
	{
		PopupMenu p;
		p.addItem(1, "Create MIDI Mapping");
		p.addItem(2, "Create Macro Mapping");
		p.addItem(3, "Create Generic Mapping (Set as source)");
		p.addItem(4, "Create Generic Mapping (Set as destination)");

		p.showMenuAsync(PopupMenu::Options().withDeletionCheck(*this), [=](int result)
			{
				Mapping* m = nullptr;
				switch (result)
				{
				case 1: m = new MIDIMapping(); break;
				case 2: m = new MacroMapping(); break;
				case 3:
					m = new GenericMapping(); break;
				case 4: m = new GenericMapping(); break;
				}

				if (result != 3) m->destParam->setValueFromTarget(e->controllable.get());
				else ((GenericMapping*)m)->sourceParam->setValueFromTarget(e->controllable.get());

				manager->addItem(m);
			});
	}

	BaseManagerShapeShifterUI::itemDropped(details);
}
