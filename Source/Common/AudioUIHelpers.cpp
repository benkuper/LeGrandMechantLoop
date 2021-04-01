/*
  ==============================================================================

    AudioUIHelpers.cpp
    Created: 26 Nov 2020 11:35:59am
    Author:  bkupe

  ==============================================================================
*/

#include "AudioUIHelpers.h"

DecibelSliderUI::DecibelSliderUI(FloatParameter* p) :
    FloatSliderUI(p)
{
    
}

DecibelSliderUI::~DecibelSliderUI()
{
}

String DecibelSliderUI::getValueText() const
{
    return Decibels::toString(Decibels::gainToDecibels(parameter->floatValue()));
}


RMSSliderUI::RMSSliderUI(FloatParameter* p) :
    FloatSliderUI(p)
{
    updateRate = 10;
    showLabel = false;
    showValue = false;
}
