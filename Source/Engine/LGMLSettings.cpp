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
    animateConnectionIntensity = addBoolParameter("Animate Connections Intensity", "If checked, this will animate the connection wires in the Node View", true);
    autoLearnOnCreateMapping = addBoolParameter("Auto Learn On Create Mapping", "If checked, this will automatically activate learn of a mapping when creating it", true);
}

LGMLSettings::~LGMLSettings()
{
}
