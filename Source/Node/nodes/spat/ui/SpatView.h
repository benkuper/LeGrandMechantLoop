/*
  ==============================================================================

    SpatView.h
    Created: 21 Nov 2020 6:32:59pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "SpatItemUI.h"

class SpatView :
    public Component
{
public:
    SpatView(SpatProcessor * item);
    ~SpatView();

    SpatProcessor * proc;
    class SpatTarget :
        public Component
    {
    public:
        SpatTarget() {}
        ~SpatTarget() {}


        void paint(Graphics& g) override;
    };

    SpatTarget target;
    OwnedArray<SpatItemUI> itemsUI;
    Point<float> posAtMouseDown;

    void paint(Graphics& g) override;
    void resized() override;

    void updateItemsUI();
    void placeItem(SpatItemUI* ui);
    SpatItemUI* getUIForItem(SpatItem* item);

    void mouseDown(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;
};