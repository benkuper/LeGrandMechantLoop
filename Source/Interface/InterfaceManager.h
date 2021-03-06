/*
  ==============================================================================

    InterfaceManager.h
    Created: 15 Nov 2020 8:43:43am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "Interface.h"

class InterfaceManager :
    public BaseManager<Interface>
{
public:
    juce_DeclareSingleton(InterfaceManager, true);
    InterfaceManager();
    ~InterfaceManager();
};