/*
  ==============================================================================

    AudioUIHelpers.h
    Created: 26 Nov 2020 11:35:59am
    Author:  bkupe

  ==============================================================================
*/

#pragma once
#include "JuceHeader.h"

class RMSSliderUI :
    public FloatSliderUI
{
public:
    RMSSliderUI(FloatParameter * p);
    ~RMSSliderUI() {}
};
