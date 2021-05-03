/*
  ==============================================================================

    InterfaceUI.h
    Created: 15 Nov 2020 9:26:44am
    Author:  bkupe

  ==============================================================================
*/

#pragma once
#include "../Interface.h"

class InterfaceUI :
    public BaseItemUI<Interface>
{
public:
    InterfaceUI(Interface* i);
    virtual ~InterfaceUI();
};