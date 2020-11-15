/*
  ==============================================================================

    TransportUI.h
    Created: 15 Nov 2020 8:53:53am
    Author:  bkupe

  ==============================================================================
*/

#pragma once
#include "../Transport.h"

class TransportUI :
    public ShapeShifterContentComponent
{
public:
    TransportUI(StringRef name);
    ~TransportUI();

    static TransportUI* create(const String& name) { return new TransportUI(name); }
};