/*
  ==============================================================================

    Transport.cpp
    Created: 15 Nov 2020 8:53:43am
    Author:  bkupe

  ==============================================================================
*/

#include "Transport.h"

juce_ImplementSingleton(Transport)

Transport::Transport() :
    ControllableContainer("Transport")
{
}

Transport::~Transport()
{
}
