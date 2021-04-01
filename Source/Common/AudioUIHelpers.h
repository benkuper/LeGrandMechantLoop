/*
  ==============================================================================

    AudioUIHelpers.h
    Created: 26 Nov 2020 11:35:59am
    Author:  bkupe

  ==============================================================================
*/

#pragma once
#include "JuceHeader.h"

class DecibelSliderUI :
    public FloatSliderUI
{
public:
    DecibelSliderUI(FloatParameter* p);
    ~DecibelSliderUI();

    virtual String getValueText() const override;
};

class RMSSliderUI :
    public FloatSliderUI
{
public:
    RMSSliderUI(FloatParameter * p);
    ~RMSSliderUI() {}
};
