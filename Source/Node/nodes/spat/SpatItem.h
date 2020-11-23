/*
  ==============================================================================

    SpatItem.h
    Created: 21 Nov 2020 6:32:24pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "../../NodeAudioProcessor.h"

class SpatProcessor;

class SpatItem :
	public ControllableContainer
{
public:
	SpatItem(SpatProcessor* processor, int index);
	~SpatItem();

	SpatProcessor* processor;

	int index;

	Point2DParameter* position;
	FloatParameter* radius;
	FloatParameter* weight;
	FloatParameter* rms;
};
