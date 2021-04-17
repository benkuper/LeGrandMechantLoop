/*
  ==============================================================================

    LGMLOutliner.cpp
    Created: 17 Apr 2021 5:36:16pm
    Author:  bkupe

  ==============================================================================
*/

#include "LGMLOutliner.h"

LGMLOutlinerItemComponent::LGMLOutlinerItemComponent(LGMLOutlinerItem* item) :
    OutlinerItemComponent(item),
    lgmlItem(item)
{
    if (!item->isContainer && item->controllable->type != Controllable::TRIGGER)
    {
        presetBT.reset(AssetManager::getInstance()->getToggleBTImage(ImageCache::getFromMemory(BinaryData::p_png, BinaryData::p_pngSize)));
        presetBT->setToggleState(lgmlItem->isPresettable(), dontSendNotification);
        presetBT->addListener(this);
        addAndMakeVisible(presetBT.get());
    }
}

LGMLOutlinerItemComponent::~LGMLOutlinerItemComponent()
{
}

void LGMLOutlinerItemComponent::paint(Graphics& g)
{
    OutlinerItemComponent::paint(g);
}

void LGMLOutlinerItemComponent::resizedInternal(Rectangle<int>& r)
{
    if(presetBT != nullptr) presetBT->setBounds(r.removeFromRight(r.getHeight()).reduced(4));
}

void LGMLOutlinerItemComponent::buttonClicked(Button* b)
{
    OutlinerItemComponent::buttonClicked(b);

    if (presetBT != nullptr && b == presetBT.get())
    {
        lgmlItem->togglePresettable();
        presetBT->setToggleState(lgmlItem->isPresettable(), dontSendNotification);
    }
}

LGMLOutlinerItem::LGMLOutlinerItem(WeakReference<ControllableContainer> container, bool parentsHaveHideInRemote, bool isFiltered) :
    OutlinerItem(container, parentsHaveHideInRemote, isFiltered)
{

}

LGMLOutlinerItem::LGMLOutlinerItem(WeakReference<Controllable> controllable, bool parentsHaveHideInRemote, bool isFiltered) :
    OutlinerItem(controllable, parentsHaveHideInRemote, isFiltered)
{

}

LGMLOutlinerItem::~LGMLOutlinerItem()
{
}

void LGMLOutlinerItem::togglePresettable()
{
    Inspectable* i = isContainer ? (Inspectable *)container.get() : (Inspectable *)controllable.get();
    if (isPresettable()) i->customData = var();
    else i->customData = "presettable";
    i->saveCustomData = true;
}

bool LGMLOutlinerItem::isPresettable()
{
    Inspectable* i = isContainer ? (Inspectable*)container.get() : (Inspectable*)controllable.get();
    return i->customData == "presettable";
}

Component* LGMLOutlinerItem::createItemComponent()
{
    return new LGMLOutlinerItemComponent(this);
}

LGMLOutliner::LGMLOutliner(const String& name) :
    Outliner(name)
{
}

LGMLOutliner::~LGMLOutliner()
{
}

OutlinerItem* LGMLOutliner::createItem(WeakReference<ControllableContainer> container, bool parentsHaveHideInRemote, bool isFiltered)
{
    return new LGMLOutlinerItem(container, parentsHaveHideInRemote, isFiltered);
}


OutlinerItem* LGMLOutliner::createItem(WeakReference<Controllable> controllable, bool parentsHaveHideInRemote, bool isFiltered)
{
    return new LGMLOutlinerItem(controllable, parentsHaveHideInRemote, isFiltered);
}
