/*
  ==============================================================================

    Transport.h
    Created: 15 Nov 2020 8:53:43am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"

class Transport :
    public ControllableContainer
{
public:
    juce_DeclareSingleton(Transport, true);

    Transport();
    ~Transport();
};