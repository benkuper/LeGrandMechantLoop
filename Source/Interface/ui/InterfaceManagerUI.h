/*
  ==============================================================================

    InterfaceManagerUI.h
    Created: 15 Nov 2020 9:26:48am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "../InterfaceManager.h"
#include "InterfaceUI.h"

class InterfaceManagerUI :
    public BaseManagerShapeShifterUI<InterfaceManager, Interface, InterfaceUI>
{
public:
    InterfaceManagerUI(StringRef name);
    ~InterfaceManagerUI();

    static InterfaceManagerUI* create(const String& name) { return new InterfaceManagerUI(name); }
};