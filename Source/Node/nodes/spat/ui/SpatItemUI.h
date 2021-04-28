/*
  ==============================================================================

    SpatItemUI.h
    Created: 21 Nov 2020 6:32:53pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

class SpatItemUI :
    public InspectableContentComponent,
    public ContainerAsyncListener
{
public:
    SpatItemUI(SpatItem* item);
    ~SpatItemUI();

    SpatItem* item;
    Point<float> posAtMouseDown;

    void mouseDown(const MouseEvent& e) override;
    bool hitTest(int x, int y) override;
    void paint(Graphics& g) override;


    void newMessage(const ContainerAsyncEvent& e) override;
};