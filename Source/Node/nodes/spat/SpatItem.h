/*
  ==============================================================================

	SpatItem.h
	Created: 21 Nov 2020 6:32:24pm
	Author:  bkupe

  ==============================================================================
*/

#pragma once

class SpatItem :
	public BaseItem
{
public:
	SpatItem(const String& name = "SpatItem", var params = var());
	virtual ~SpatItem();

	int index;

	Point2DParameter* position;
	FloatParameter* radius;

	void setIndex(int i) { index = i; setNiceName(getTypeString() + " " + String(i)); }
};


class SpatSource :
	public SpatItem
{
public:
	SpatSource();
	~SpatSource();

	enum ControlMode { FREE_2D, FREE_POLAR, CONTROL_CIRCLE, CONTROL_NOISE };
	EnumParameter* controlMode;

	siv::PerlinNoise noise;

	SpinLock controlLock;
	Trigger* controlResetTime;
	FloatParameter* controlSpeed;
	FloatParameter* controlAngle;
	FloatParameter* controlRadius;
	Array<Parameter*> controlParams;

	float controlCurTime;
	float lastControlUpdate;

	void onContainerTriggerTriggered(Trigger* t) override;
	void onContainerParameterChangedInternal(Parameter* p) override;

	void updateControlMode();
	void update();

	DECLARE_TYPE("Source");
};

class SpatTarget :
	public SpatItem
{
public:
	SpatTarget();
	~SpatTarget();


	Array<FloatParameter*> weights;
	Array<Colour> colors;
	FloatParameter* rms;

	void setSources(Array<Colour> colors);

	DECLARE_TYPE("Target");
};

