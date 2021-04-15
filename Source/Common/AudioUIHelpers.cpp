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

float DecibelSliderUI::getNormalizedValueFromMouseDrag(const MouseEvent &e)
{
    float initValPos = getDrawPosForValue(initValue); 
    if (orientation == VERTICAL) initValPos = getHeight() - initValPos;
    float offset = orientation == HORIZONTAL ? e.getOffsetFromDragStart().x : e.getOffsetFromDragStart().y;

    float scaleFactor = e.mods.isAltDown() ? .5f : 1;
    float targetPos = initValPos + offset * scaleFactor;

    float relVal = orientation == HORIZONTAL ? targetPos * 1.0f / getWidth() : 1 - (targetPos * 1.0f / getHeight());

    float mapVal = jmap<float>(relVal, -100, 6);
    float val = Decibels::decibelsToGain(mapVal);
    float normVal = jmap<float>(val, 0, 2, 0, 1);

    return normVal;
}

int DecibelSliderUI::getDrawPos()
{
    return getDrawPosForValue(parameter->floatValue());
}

int DecibelSliderUI::getDrawPosForValue(float val)
{
    float decVal = Decibels::gainToDecibels(val);
    float mapVal = jmap<float>(decVal, -100, 6, 0, 1);
    if (orientation == HORIZONTAL) return mapVal * getWidth();
    else return mapVal * getHeight();
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
