/*
  ==============================================================================

	SpatNode.h
	Created: 15 Nov 2020 8:42:35am
	Author:  bkupe

  ==============================================================================
*/

#pragma once

class SpatNode :
	public Node
{
public:
	SpatNode(var params = var());
	~SpatNode() {}

	ControllableContainer layoutCC;

	enum SpatMode { FREE, CIRCLE };
	EnumParameter* spatMode;

	FloatParameter* spatRadius;
	Automation fadeCurve;
	FloatParameter* circleRadius;
	FloatParameter* circleAngle;

	ControllableContainer positionCC;

	enum ControlMode { FREE_2D, FREE_POLAR, CONTROL_CIRCLE, CONTROL_NOISE };
	EnumParameter* controlMode;
	Point2DParameter* position;
	siv::PerlinNoise noise;

	//parameters
	Trigger* controlResetTime;
	FloatParameter* controlSpeed;
	FloatParameter* controlAngle;
	FloatParameter* controlRadius;
	Array<Parameter*> controlParams;

	float controlCurTime;
	float lastControlUpdate;

	ControllableContainer spatCC;

	AudioBuffer<float> tmpBuffer;

	void updateAudioOutputsInternal() override;

	void updateSpatPoints();
	void updateControlMode();

	void placeItems();
	void updateRadiuses();

	void onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c) override;

	void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

	void updateSpatControl();

	String getTypeString() const override { return getTypeStringStatic(); }
	static const String getTypeStringStatic() { return "Spat"; }

	BaseNodeViewUI* createViewUI() override;
};
