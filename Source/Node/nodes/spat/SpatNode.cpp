/*
  ==============================================================================

	SpatNode.cpp
	Created: 15 Nov 2020 8:42:35am
	Author:  bkupe

  ==============================================================================
*/

#include "Node/NodeIncludes.h"

SpatNode::SpatNode(var params) :
	Node(getTypeString(), params, true, true, true, true),
	fadeCurve("Fade Curve", nullptr),
	layoutCC("Layout"),
	positionCC("Positionning"),
	spatCC("Points"),
	controlSpeed(nullptr),
	controlResetTime(nullptr),
	controlAngle(nullptr),
	controlRadius(nullptr),
	controlCurTime(0),
	lastControlUpdate(0)
{
	saveAndLoadRecursiveData = true;

	spatMode = layoutCC.addEnumParameter("Mode", "Mode of the spat");
	spatMode->addOption("Circle", CIRCLE)->addOption("Free", FREE);

	spatRadius = layoutCC.addFloatParameter("Radius", "Radius to compute the weights from", .5f, 0, 1);

	AutomationKey* k = fadeCurve.addKey(0, 1);
	k->easingType->setValueWithData(Easing::BEZIER);
	fadeCurve.addKey(1, 0);
	fadeCurve.editorCanBeCollapsed = true;
	fadeCurve.editorIsCollapsed = true;
	layoutCC.addChildControllableContainer(&fadeCurve);

	circleRadius = layoutCC.addFloatParameter("Circle Radius", "Radius of the circle if in circle mode", .8f, 0, 2);
	circleAngle = layoutCC.addFloatParameter("Circle Angle", "Angle of the circle if in cirlce mode", 0, 0, 360);

	addChildControllableContainer(&layoutCC);

	controlMode = positionCC.addEnumParameter("Mode", "Mode of the positionning");
	controlMode->addOption("Free 2D", FREE_2D)->addOption("Free Polar", FREE_POLAR)->addOption("Circle", CONTROL_CIRCLE)->addOption("Noise", CONTROL_NOISE);

	position = positionCC.addPoint2DParameter("Position", "Position to compute the weights from");
	position->setBounds(0, 0, 1, 1);
	position->setPoint(.5f, .5f);


	addChildControllableContainer(&positionCC);


	spatCC.editorIsCollapsed = true;
	addChildControllableContainer(&spatCC);

	viewUISize->setPoint(300, 300);
}

void SpatNode::updateAudioOutputsInternal()
{
	updateSpatPoints();
}

void SpatNode::updateSpatPoints()
{
	jassert(processor->isSuspended());
	DBG("Num spat items before updates : " << spatCC.controllableContainers.size() << " / " << getNumAudioOutputs());

	while (spatCC.controllableContainers.size() > getNumAudioOutputs())
	{
		spatCC.removeChildControllableContainer(spatCC.controllableContainers[spatCC.controllableContainers.size() - 1]);
	}

	while (spatCC.controllableContainers.size() < getNumAudioOutputs())
	{
		int index = spatCC.controllableContainers.size();
		SpatItem* si = new SpatItem(index);
		spatCC.addChildControllableContainer(si, true);
	}

	placeItems();
	updateRadiuses();


	jassert(processor->isSuspended());

	DBG("Num spat items after updates : " << spatCC.controllableContainers.size() << " / " << getNumAudioOutputs());
}

void SpatNode::updateControlMode()
{
	if (controlResetTime) positionCC.removeControllable(controlResetTime);
	if (controlSpeed) positionCC.removeControllable(controlSpeed);
	if (controlAngle) positionCC.removeControllable(controlAngle);
	if (controlRadius) positionCC.removeControllable(controlRadius);
	controlResetTime = nullptr;
	controlSpeed = nullptr;
	controlAngle = nullptr;
	controlRadius = nullptr;

	for (auto& p : controlParams) positionCC.removeControllable(p);
	controlParams.clear();

	ControlMode m = controlMode->getValueDataAsEnum<ControlMode>();

	if (m != FREE_2D && m != FREE_POLAR)
	{
		controlResetTime = positionCC.addTrigger("Reset", "Reset the animation to the start");
		controlSpeed = positionCC.addFloatParameter("Speed", "Speed of the animation, in seconds per cycle", 1);
	}

	if (m == FREE_POLAR || m == CONTROL_CIRCLE)
	{
		controlAngle = positionCC.addFloatParameter("Angle", "Angle of the positionning. 0-1 for one full revolution", 0, 0, 1);
		controlRadius = positionCC.addFloatParameter("Radius", "Radius of the positionning.", .5f, 0, 1);
	}

	position->setControllableFeedbackOnly(m != FREE_2D && m != FREE_POLAR);

	switch (m)
	{
	case FREE_2D:
		break;

	case FREE_POLAR:
		break;

	case CONTROL_CIRCLE:
	{
		Point2DParameter* axisMult = positionCC.addPoint2DParameter("Axis Multiplier", "To create axis-aligned ellipsed and pure horizontal or vertical animations");
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
		controlParams.add(positionCC.addFloatParameter("Amplitude", "Amount of noise to apply to the position", 0, 0, 1));
	}
	break;
	default:
		break;
	}
}

void SpatNode::placeItems()
{
	SpatMode m = spatMode->getValueDataAsEnum<SpatMode>();
	switch (m)
	{
	case CIRCLE:
	{
		int numItems = spatCC.controllableContainers.size();
		for (int i = 0; i < numItems; i++)
		{
			float rel = i * 1.0f / numItems;
			float rad = circleRadius->floatValue();

			float angle = (circleAngle->floatValue() + rel * 360 + 180) * float_Pi / 180;
			SpatItem* si = (SpatItem*)spatCC.controllableContainers[i].get();
			si->position->setPoint(.5f + cosf(angle) * .5f * rad, .5f + sinf(angle) * .5f * rad);
		}
	}
	break;


	default:
		break;
	}

}

void SpatNode::updateRadiuses()
{
	int numItems = spatCC.controllableContainers.size();
	for (int i = 0; i < numItems; i++)
	{
		SpatItem* si = (SpatItem*)spatCC.controllableContainers[i].get();
		si->radius->defaultValue = spatRadius->floatValue();
		if (!si->radius->isOverriden) si->radius->resetValue();
	}
}


void SpatNode::onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c)
{
	if (c == spatMode || c == circleRadius || c == circleAngle)
	{
		if (c == spatMode)
		{
			SpatMode sm = spatMode->getValueDataAsEnum<SpatMode>();
			circleRadius->setEnabled(sm == CIRCLE);
			circleAngle->setEnabled(sm == CIRCLE);
		}

		placeItems();
	}
	else if (c == spatRadius)
	{
		updateRadiuses();
	}
	else if (c == controlMode)
	{
		updateControlMode();
	}
	else if (c == controlResetTime)
	{
		controlCurTime = 0;
		lastControlUpdate = Time::getMillisecondCounter() / 1000.0f;
	}
	else if (c == controlRadius || c == controlAngle)
	{
		ControlMode m = controlMode->getValueDataAsEnum<ControlMode>();
		if (m == FREE_POLAR)
		{
			float rad = controlRadius->floatValue();
			float angle = controlAngle->floatValue() * MathConstants<float>::pi * 2;
			float x = cosf(angle) * .5f * rad + .5f;
			float y = sinf(angle) * .5f * rad + .5f;

			position->setPoint(x, y);
		}

	}
}

void SpatNode::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
	int numSamples = buffer.getNumSamples();

	updateSpatControl();

	jassert(getNumAudioOutputs() == spatCC.controllableContainers.size());

	int numOutputs = spatCC.controllableContainers.size();

	tmpBuffer.setSize(numOutputs, numSamples);
	tmpBuffer.clear();


	for (int outputIndex = 0; outputIndex < numOutputs; outputIndex++)
	{
		SpatItem* si = (SpatItem*)spatCC.controllableContainers[outputIndex].get();
		float dist = position->getPoint().getDistanceFrom(si->position->getPoint());
		float rad = si->radius->floatValue();
		float relDist = jlimit<float>(0, 1, rad == 0 ? 1 : dist / rad);
		float weight = fadeCurve.getValueAtPosition(relDist);

		for (int inputIndex = 0; inputIndex < getNumAudioInputs(); inputIndex++)
		{
			tmpBuffer.addFrom(outputIndex, 0, buffer.getReadPointer(inputIndex), numSamples, weight);
		}

		si->weight->setValue(weight);
	}

	for (int outputIndex = 0; outputIndex < numOutputs; outputIndex++)
	{
		buffer.clear(outputIndex, 0, numSamples);
		buffer.addFrom(outputIndex, 0, tmpBuffer.getReadPointer(outputIndex), numSamples);

		jassert(!processor->isSuspended());
		SpatItem* si = (SpatItem*)spatCC.controllableContainers[outputIndex].get();

		float rms = buffer.getRMSLevel(outputIndex, 0, buffer.getNumSamples());
		float curVal = si->rms->floatValue();
		float targetVal = si->rms->getLerpValueTo(rms, rms > curVal ? .8f : .2f);
		si->rms->setValue(targetVal); //to change
	}
}

void SpatNode::updateSpatControl()
{
	ControlMode m = controlMode->getValueDataAsEnum<ControlMode>();

	if (m == FREE_2D || m == FREE_POLAR) return;

	if (controlSpeed == nullptr) return;

	float t = Time::getMillisecondCounter() / 1000.0f;
	float dt = t - lastControlUpdate;
	controlCurTime += controlSpeed->floatValue() * dt;
	lastControlUpdate = t;

	switch (m)
	{
	case CONTROL_CIRCLE:
	{
		float rad = controlRadius->floatValue();
		float angle = (controlRadius->floatValue() + controlCurTime) * MathConstants<float>::pi * 2;
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

BaseNodeViewUI* SpatNode::createViewUI()
{
	return new SpatNodeViewUI(this);
}
