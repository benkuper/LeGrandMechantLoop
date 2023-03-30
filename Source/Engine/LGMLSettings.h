/*
  ==============================================================================

    LGMLSettings.h
    Created: 15 Nov 2020 8:45:19am
    Author:  bkupe

  ==============================================================================
*/

#pragma once
#include "JuceHeader.h"

class LGMLSettings :
    public ControllableContainer
{
public:
    juce_DeclareSingleton(LGMLSettings, true);

    BoolParameter* animateConnectionIntensity;
    BoolParameter* autoLearnOnCreateMapping;

    LGMLSettings();
    ~LGMLSettings();
};