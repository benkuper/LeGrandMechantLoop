/*
  ==============================================================================

    LGMLOutliner.h
    Created: 17 Apr 2021 5:36:16pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"

class LGMLOutlinerItem;
class LGMLOutliner;

class LGMLOutlinerItemComponent :
    public OutlinerItemComponent
{
public:
    LGMLOutlinerItemComponent(LGMLOutlinerItem* item);
    virtual ~LGMLOutlinerItemComponent();

    std::unique_ptr<ImageButton> presetBT;

    LGMLOutlinerItem * lgmlItem;

    void paint(Graphics& g) override;
    void resizedInternal(Rectangle<int> &r) override;
    
    void buttonClicked(Button* b) override;

};

class LGMLOutlinerItem :
    public OutlinerItem
{
public:
    LGMLOutlinerItem(WeakReference<ControllableContainer> container, bool parentsHaveHideInRemote, bool isFiltered);
    LGMLOutlinerItem(WeakReference<Controllable> controllable, bool parentsHaveHideInRemote, bool isFiltered);
    virtual ~LGMLOutlinerItem();

    std::unique_ptr<Component> createItemComponent() override;

};

class LGMLOutliner :
    public Outliner
{
public:
    LGMLOutliner(const String& name);
    ~LGMLOutliner();

    virtual OutlinerItem* createItem(WeakReference<ControllableContainer> container, bool parentsHaveHideInRemote, bool isFiltered);
    virtual OutlinerItem* createItem(WeakReference<Controllable> controllable, bool parentsHaveHideInRemote, bool isFiltered);
};