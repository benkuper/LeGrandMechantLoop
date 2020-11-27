/*
  ==============================================================================

    AudioUIHelpers.cpp
    Created: 26 Nov 2020 11:35:59am
    Author:  bkupe

  ==============================================================================
*/

#include "AudioUIHelpers.h"

RMSSliderUI::RMSSliderUI(FloatParameter* p) :
    FloatSliderUI(p)
{
    updateRate = 15;
    orientation = VERTICAL;
    showLabel = false;
    showValue = false;
}
