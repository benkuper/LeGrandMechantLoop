/*
  ==============================================================================

    InterfaceManagerUI.cpp
    Created: 15 Nov 2020 9:26:48am
    Author:  bkupe

  ==============================================================================
*/

#include "InterfaceManagerUI.h"

InterfaceManagerUI::InterfaceManagerUI(StringRef name) :
    BaseManagerShapeShifterUI(name, InterfaceManager::getInstance())
{
    addExistingItems();

}

InterfaceManagerUI::~InterfaceManagerUI()
{
}
