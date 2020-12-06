/*
  ==============================================================================

    SpatNode.h
    Created: 15 Nov 2020 8:42:35am
    Author:  bkupe

  ==============================================================================
*/

#pragma once
#include "../../Node.h"
#include "SpatItem.h"

class SpatNode :
	public Node
{
public:
	SpatNode(var params = var());
	~SpatNode() {}

	enum SpatMode { FREE, CIRCLE };
	EnumParameter* spatMode;

	Point2DParameter* spatPosition;
	
	FloatParameter* spatRadius;
	Automation fadeCurve;

	FloatParameter* circleRadius;
	FloatParameter* circleAngle;

	ControllableContainer spatCC;

	AudioBuffer<float> tmpBuffer;

	void updateAudioOutputsInternal() override;

	void updateSpatPoints();

	void placeItems();
	void updateRadiuses();

	void onContainerParameterChangedInternal(Parameter* p) override;

	void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;
	
	String getTypeString() const override { return getTypeStringStatic(); }
	static const String getTypeStringStatic() { return "Spat"; }

	BaseNodeViewUI* createViewUI() override;
};
