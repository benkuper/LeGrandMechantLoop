/*
  ==============================================================================

    LGMLSettings.cpp
    Created: 15 Nov 2020 8:45:19am
    Author:  bkupe

  ==============================================================================
*/

#include "LGMLSettings.h"

juce_ImplementSingleton(LGMLSettings)

LGMLSettings::LGMLSettings() :
    ControllableContainer("LGML")
{
}

LGMLSettings::~LGMLSettings()
{
}
