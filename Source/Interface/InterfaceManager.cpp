/*
  ==============================================================================

    InterfaceManager.cpp
    Created: 15 Nov 2020 8:43:43am
    Author:  bkupe

  ==============================================================================
*/

#include "InterfaceManager.h"

juce_ImplementSingleton(InterfaceManager)

InterfaceManager::InterfaceManager() :
    BaseManager("Interfaces")
{
}

InterfaceManager::~InterfaceManager()
{
}
