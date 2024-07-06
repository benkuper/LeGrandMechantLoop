/*
  ==============================================================================

	SpatItem.cpp
	Created: 21 Nov 2020 6:32:24pm
	Author:  bkupe

  ==============================================================================
*/

#include "Node/NodeIncludes.h"

SpatItem::SpatItem(const String& name, var params) :
	BaseItem("Point"),
    index(0)
{
	setHasCustomColor(true);
	if(!Engine::mainEngine->isLoadingFile) itemColor->setColor(Colour::fromHSV(Random().nextFloat(), .8f, .8f, 1));
	//hideInEditor = true;
	position = addPoint2DParameter("Position", "Position of this point");
	position->setBounds(0, 0, 1, 1);

}

SpatItem::~SpatItem()
{
}


SpatSource::SpatSource() :
	SpatItem(getTypeString()),
	controlSpeed(nullptr),
	controlResetTime(nullptr),
	controlAngle(nullptr),
	controlRadius(nullptr),
	controlCurTime(0),
	lastControlUpdate(0)
{
	setHasCustomColor(true);

	Random r;
	itemColor->setColor(Colour::fromHSV(r.nextFloat(), .8f, .5f, 1));

	controlMode = addEnumParameter("Mode", "Mode of the positionning");
	controlMode->addOption("Free 2D", FREE_2D)->addOption("Free Polar", FREE_POLAR)->addOption("Circle", CONTROL_CIRCLE)->addOption("Noise", CONTROL_NOISE);

	var val;
	val.append(.5f);
	val.append(.5f);
	position->setDefaultValue(val);
}

SpatSource::~SpatSource()
{
}

void SpatSource::onContainerTriggerTriggered(Trigger* t)
{
	if (t == controlResetTime)
	{
		GenericScopedLock lock(controlLock);
		controlCurTime = 0;
		lastControlUpdate = Time::getMillisecondCounter() / 1000.0f;
	}
}

void SpatSource::onContainerParameterChangedInternal(Parameter* p)
{
	if (p == controlMode)
	{
		updateControlMode();
	}
	else if (p == controlRadius || p == controlAngle)
	{
		ControlMode m = controlMode->getValueDataAsEnum<ControlMode>();
		if (m == FREE_POLAR)
		{
			GenericScopedLock lock(controlLock);

			float rad = controlRadius->floatValue();
			float angle = controlAngle->floatValue() * MathConstants<float>::pi * 2;
			float x = cosf(angle) * .5f * rad + .5f;
			float y = sinf(angle) * .5f * rad + .5f;

			position->setPoint(x, y);
		}

	}
}

void SpatSource::updateControlMode()
{
	GenericScopedLock lock(controlLock);

	if (controlResetTime) removeControllable(controlResetTime);
	if (controlSpeed) removeControllable(controlSpeed);
	if (controlAngle) removeControllable(controlAngle);
	if (controlRadius) removeControllable(controlRadius);
	controlResetTime = nullptr;
	controlSpeed = nullptr;
	controlAngle = nullptr;
	controlRadius = nullptr;

	for (auto& p : controlParams) removeControllable(p);
	controlParams.clear();

	ControlMode m = controlMode->getValueDataAsEnum<ControlMode>();

	if (m != FREE_2D && m != FREE_POLAR)
	{
		controlResetTime = addTrigger("Reset", "Reset the animation to the start");
		controlSpeed = addFloatParameter("Speed", "Speed of the animation, in seconds per cycle", 1);
	}

	if (m == FREE_POLAR || m == CONTROL_CIRCLE)
	{
		controlAngle = addFloatParameter("Angle", "Angle of the positionning. 0-1 for one full revolution", 0, 0, 1);
		controlRadius = addFloatParameter("Radius", "Radius of the positionning.", .5f, 0, 1);
	}

	position->setControllableFeedbackOnly(m != FREE_2D);

	switch (m)
	{
	case FREE_2D:
		break;

	case FREE_POLAR:
		break;

	case CONTROL_CIRCLE:
	{
		Point2DParameter* axisMult = addPoint2DParameter("Axis Multiplier", "To create axis-aligned ellipsed and pure horizontal or vertical animations");
		var defVal;
		defVal.append(1);
		defVal.append(1);
		axisMult->setDefaultValue(defVal);
		axisMult->setBounds(0, 0, 1, 1);
		controlParams.add(axisMult);
	}
	break;
	case CONTROL_NOISE:
	{
		controlParams.add(addFloatParameter("Amplitude", "Amount of noise to apply to the position", 0, 0, 1));
	}
	break;
	default:
		break;
	}
}

void SpatSource::update()
{
	ControlMode m = controlMode->getValueDataAsEnum<ControlMode>();

	if (m == FREE_2D || m == FREE_POLAR) return;

	GenericScopedLock lock(controlLock);
	float t = Time::getMillisecondCounter() / 1000.0f;
	float dt = t - lastControlUpdate;
	controlCurTime += controlSpeed->floatValue() * dt;
	lastControlUpdate = t;

	switch (m)
	{
	case CONTROL_CIRCLE:
	{
		float rad = controlRadius->floatValue();
		float angle = (controlAngle->floatValue() + controlCurTime) * MathConstants<float>::pi * 2;
		angle += controlCurTime;

		Point<float> axisMult = ((Point2DParameter*)controlParams[0])->getPoint();

		float x = cosf(angle) * .5f * rad * axisMult.x + .5f;
		float y = sinf(angle) * .5f * rad * axisMult.y + .5f;

		position->setPoint(x, y);
	}
	break;

	case CONTROL_NOISE:
	{
		Point<float> p = position->getPoint();
		float x = noise.noise0_1(p.x + controlCurTime);
		float y = noise.noise0_1(p.y + controlCurTime + 1);

		float amp = controlParams[0]->floatValue();
		x = jmap(amp, .5f, x);
		y = jmap(amp, .5f, y);
		position->setPoint(x, y);
	}
	break;
	}
}


SpatTarget::SpatTarget() :
	SpatItem(getTypeString())
{
	radius = addFloatParameter("Radius", "Radius of this point. If not overriden, this will be synced with the global radius.", .1f, 0, 1);
	
	rms = addFloatParameter("RMS", "Computed RMS of this point", 0, 0, 1);
	rms->setControllableFeedbackOnly(true);
}

SpatTarget::~SpatTarget()
{
}

void SpatTarget::setSources(Array<Colour> newColors)
{
	for (auto& p : weights) removeControllable(p);
	weights.clear();
	colors.clear();
	for (auto& c : newColors)
	{
		colors.add(c);
		FloatParameter* p = addFloatParameter("Weight", "Computed weight of this point", 0, 0, 1);
		p->setControllableFeedbackOnly(true);
		weights.add(p);
	}
}
