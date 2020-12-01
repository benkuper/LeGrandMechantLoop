/*
  ==============================================================================

    SpatNode.h
    Created: 15 Nov 2020 8:42:35am
    Author:  bkupe

  ==============================================================================
*/

#pragma once
#include "SpatItem.h"

class SpatProcessor :
	public GenericNodeProcessor
{
public:
	SpatProcessor(Node* node);
	~SpatProcessor() {}

	IntParameter* numInputs;
	IntParameter* numOutputs;

	enum SpatMode { FREE, CIRCLE };
	EnumParameter* spatMode;

	Point2DParameter* spatPosition;
	
	FloatParameter* spatRadius;
	Automation fadeCurve;

	FloatParameter* circleRadius;
	FloatParameter* circleAngle;

	ControllableContainer spatCC;

	AudioBuffer<float> tmpBuffer;

	void updateOutputsFromNodeInternal() override;

	void updateSpatPoints();

	void placeItems();
	void updateRadiuses();

	void onContainerParameterChanged(Parameter* p) override;

	void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;
	static const String getTypeStringStatic() { return "Spat"; }

	NodeViewUI* createNodeViewUI() override;
};
