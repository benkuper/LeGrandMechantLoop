/*
  ==============================================================================

    SpatNode.cpp
    Created: 15 Nov 2020 8:42:35am
    Author:  bkupe

  ==============================================================================
*/

#include "SpatNode.h"
#include "ui/SpatNodeViewUI.h"

SpatProcessor::SpatProcessor(Node* node) :
    GenericNodeProcessor(node),
    spatCC("Points"),
	fadeCurve("Fade Curve",nullptr)
{
	nodeRef->viewUISize->setPoint(300, 300);

	numInputs = addIntParameter("Inputs", "Number of inputs", 2, 1, 32);
	numOutputs = addIntParameter("Outputs", "Number of outputs", 6, 1, 32);

	spatMode = addEnumParameter("Mode", "Mode of the spat");
	spatMode->addOption("Circle", CIRCLE)->addOption("Free", FREE);

	spatPosition = addPoint2DParameter("Position", "Position to compute the weights from");
	spatPosition->setBounds(0, 0, 1, 1);
	spatPosition->setPoint(.5f, .5f);

	spatRadius = addFloatParameter("Radius", "Radius to compute the weights from", .5f, 0, 1);
	
	AutomationKey * k = fadeCurve.addKey(0,1);
	k->easingType->setValueWithData(Easing::BEZIER);
	fadeCurve.addKey(1, 0);
	addChildControllableContainer(&fadeCurve);

	circleRadius = addFloatParameter("Circle Radius", "Radius of the circle if in circle mode", .8f, 0, 2);
	circleAngle = addFloatParameter("Circle Angle", "Angle of the circle if in cirlce mode", 0, 0, 360);
	
	addChildControllableContainer(&spatCC);

	nodeRef->setAudioInputs(numInputs->intValue());
	nodeRef->setAudioOutputs(numOutputs->intValue());
}

void SpatProcessor::updateOutputsFromNodeInternal()
{
    updateSpatPoints();
}

void SpatProcessor::updateSpatPoints()
{
	while (spatCC.controllableContainers.size() > nodeRef->numOutputs)
	{
		spatCC.removeChildControllableContainer(spatCC.controllableContainers[spatCC.controllableContainers.size() - 1]);
	}

	while (spatCC.controllableContainers.size() < nodeRef->numOutputs)
	{
		int index = spatCC.controllableContainers.size();
		SpatItem* si = new SpatItem(this, index);
		spatCC.addChildControllableContainer(si, true);
	}

	placeItems();
	updateRadiuses();
}

void SpatProcessor::placeItems()
{
	SpatMode m = spatMode->getValueDataAsEnum<SpatMode>();
	switch (m)
	{
	case CIRCLE:
		int numItems = spatCC.controllableContainers.size();
		for (int i = 0; i < numItems; i++)
		{
			float rel = i * 1.0f / numItems;
			float rad = circleRadius->floatValue();

			float angle = (circleAngle->floatValue() + rel * 360 + 180) * float_Pi / 180;
			SpatItem* si = (SpatItem*)spatCC.controllableContainers[i].get();
			si->position->setPoint(.5f + cosf(angle) * .5f * rad, .5f + sinf(angle) * .5f * rad);
		}
		break;
	}

}

void SpatProcessor::updateRadiuses()
{
	int numItems = spatCC.controllableContainers.size();
	for (int i = 0; i < numItems; i++)
	{
		SpatItem* si = (SpatItem*)spatCC.controllableContainers[i].get();
		si->radius->defaultValue = spatRadius->floatValue();
		if (!si->radius->isOverriden) si->radius->resetValue();
	}
}

void SpatProcessor::onContainerParameterChanged(Parameter* p)
{
	if (p == numInputs) nodeRef->setAudioInputs(numInputs->intValue());
	else if (p == numOutputs) nodeRef->setAudioInputs(numOutputs->intValue());
	else if (p == spatMode || p == circleRadius || p == circleAngle)
	{
		placeItems();
	}
	else if (p == spatRadius)
	{
		updateRadiuses();
	}
}

void SpatProcessor::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
	int numSamples = buffer.getNumSamples();
	tmpBuffer.setSize(numOutputs->intValue(), numSamples);
	tmpBuffer.clear();

	for (int outputIndex = 0; outputIndex < numOutputs->intValue(); outputIndex++)
	{
		SpatItem* si = (SpatItem*)spatCC.controllableContainers[outputIndex].get();
		float dist = spatPosition->getPoint().getDistanceFrom(si->position->getPoint());
		float rad = si->radius->floatValue();
		float relDist = jlimit<float>(0, 1, rad == 0 ? 1 : dist / rad);
		float weight = fadeCurve.getValueAtPosition(relDist);

		for (int inputIndex = 0; inputIndex < numInputs->intValue(); inputIndex++)
		{
			tmpBuffer.addFrom(outputIndex, 0, buffer.getReadPointer(inputIndex), numSamples, weight);
		}

		si->weight->setValue(weight);
	}

	for (int outputIndex = 0; outputIndex < numOutputs->intValue(); outputIndex++)
	{
		buffer.clear(outputIndex, 0, numSamples);
		buffer.addFrom(outputIndex, 0, tmpBuffer.getReadPointer(outputIndex), numSamples);

		SpatItem* si = (SpatItem*)spatCC.controllableContainers[outputIndex].get();
		float rms = buffer.getRMSLevel(outputIndex, 0, buffer.getNumSamples());
		float curVal = si->rms->floatValue();
		float targetVal = si->rms->getLerpValueTo(rms, rms > curVal ? .8f : .2f);
		si->rms->setValue(targetVal);
	}
}

NodeViewUI* SpatProcessor::createNodeViewUI()
{
    return new SpatNodeViewUI((GenericNode<SpatProcessor>*)nodeRef.get());
}
