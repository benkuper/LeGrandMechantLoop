/*
  ==============================================================================

    SpatItem.cpp
    Created: 21 Nov 2020 6:32:24pm
    Author:  bkupe

  ==============================================================================
*/

#include "SpatItem.h"
#include "SpatNode.h"

SpatItem::SpatItem(SpatProcessor* processor, int index) :
	ControllableContainer("Point " + String(index + 1)),
	processor(processor),
	index(index)
{
	position = addPoint2DParameter("Position", "Position of this point");
	position->setBounds(0, 0, 1, 1);

	radius = addFloatParameter("Radius", "Radius of this point. If not overriden, this will be synced with the global radius.", .1f, 0, 1);

	weight = addFloatParameter("Weight", "Computed weight of this point", 0, 0, 1);
	weight->setControllableFeedbackOnly(true);

	rms = addFloatParameter("RMS", "Computed RMS of this point", 0, 0, 1);
	rms->setControllableFeedbackOnly(true);
}

SpatItem::~SpatItem()
{
}
