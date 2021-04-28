/*
  ==============================================================================

    SpatItem.h
    Created: 21 Nov 2020 6:32:24pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

class SpatItem :
	public ControllableContainer
{
public:
	SpatItem(int index);
	~SpatItem();

	int index;

	Point2DParameter* position;
	FloatParameter* radius;
	FloatParameter* weight;
	FloatParameter* rms;
};
