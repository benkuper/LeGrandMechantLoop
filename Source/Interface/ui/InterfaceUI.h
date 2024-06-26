/*
  ==============================================================================

    InterfaceUI.h
    Created: 15 Nov 2020 9:26:44am
    Author:  bkupe

  ==============================================================================
*/

#pragma once
class InterfaceUI :
    public BaseItemUI<Interface>
{
public:
    InterfaceUI(Interface* i);
    virtual ~InterfaceUI();

    std::unique_ptr<TriggerImageUI> inActivityUI;
    std::unique_ptr<TriggerImageUI> outActivityUI;

    void paintOverChildren(Graphics& g);
    virtual void resizedHeader(Rectangle<int>& r) override;
    virtual void mouseDown(const MouseEvent& e) override;
};