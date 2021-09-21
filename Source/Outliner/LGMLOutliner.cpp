/*
  ==============================================================================

    LGMLOutliner.cpp
    Created: 17 Apr 2021 5:36:16pm
    Author:  bkupe

  ==============================================================================
*/

#include "LGMLOutliner.h"
#include "Preset/PresetIncludes.h"

LGMLOutlinerItemComponent::LGMLOutlinerItemComponent(LGMLOutlinerItem* item) :
    OutlinerItemComponent(item),
    lgmlItem(item)
{
    if (!item->isContainer && item->controllable->type != Controllable::TRIGGER && !item->controllable->isControllableFeedbackOnly)
    {
        presetBT.reset(AssetManager::getInstance()->getToggleBTImage(ImageCache::getFromMemory(BinaryData::p_png, BinaryData::p_pngSize)));
        presetBT->setToggleState(RootPresetManager::getInstance()->isParameterPresettable((Parameter *)item->controllable.get()), dontSendNotification);
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
        RootPresetManager::getInstance()->toggleParameterPresettable((Parameter *)lgmlItem->controllable.get());
        presetBT->setToggleState(RootPresetManager::getInstance()->isParameterPresettable((Parameter*)item->controllable.get()), dontSendNotification);
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

std::unique_ptr<Component> LGMLOutlinerItem::createItemComponent()
{
    return std::unique_ptr<Component>(new LGMLOutlinerItemComponent(this));
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
