/*
  ==============================================================================

	SpatNode.cpp
	Created: 15 Nov 2020 8:42:35am
	Author:  bkupe

  ==============================================================================
*/

#include "Node/NodeIncludes.h"
#include "SpatNode.h"

SpatNode::SpatNode(var params) :
	Node(getTypeString(), params, true, true, false, true),
	fadeCurve("Fade Curve", nullptr),
	layoutCC("Layout"),
	sources("Sources"),
	targets("Targets")
{
	saveAndLoadRecursiveData = true;


	spatMode = layoutCC.addEnumParameter("Mode", "Mode of the spat");
	spatMode->addOption("Circle", CIRCLE)->addOption("Free", FREE);

	spatRadius = layoutCC.addFloatParameter("Radius", "Radius to compute the weights from", .5f, 0, 1);
	circleRadius = layoutCC.addFloatParameter("Circle Radius", "Radius of the circle if in circle mode", .8f, 0, 2);
	circleAngle = layoutCC.addFloatParameter("Circle Angle", "Angle of the circle if in cirlce mode", 0, 0, 360);

	AutomationKey* k = fadeCurve.addKey(0, 1);
	k->easingType->setValueWithData(Easing::BEZIER);
	fadeCurve.addKey(1, 0);
	fadeCurve.editorCanBeCollapsed = true;
	fadeCurve.editorIsCollapsed = true;
	layoutCC.addChildControllableContainer(&fadeCurve);

	

	addChildControllableContainer(&layoutCC);



	sources.addBaseManagerListener(this);
	targets.addBaseManagerListener(this);

	addChildControllableContainer(&sources);
	addChildControllableContainer(&targets);

	if (!Engine::mainEngine->isLoadingFile)
	{
		//add 1 source and 2 targets
		sources.addItem();
		targets.addItem();
		targets.addItem();
	}

	viewUISize->setPoint(300, 300);

}

void SpatNode::placeTargets()
{
	SpatMode m = spatMode->getValueDataAsEnum<SpatMode>();
	switch (m)
	{
	case CIRCLE:
	{
		int numItems = targets.items.size();
		for (int i = 0; i < numItems; i++)
		{
			float rel = i * 1.0f / numItems;
			float rad = circleRadius->floatValue();

			float angle = (circleAngle->floatValue() + rel * 360 + 180) * float_Pi / 180;
			SpatItem* si = targets.items[i];
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
	for (auto& si : targets.items) si->radius->setValue(spatRadius->floatValue());
}

void SpatNode::updateAudioInputsInternal()
{
	Array<Colour> colors;
	for (auto& s : sources.items) colors.add(s->itemColor->getColor());
	for (auto& t : targets.items) t->setSources(colors);
}


void SpatNode::onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c)
{
	Node::onControllableFeedbackUpdateInternal(cc, c);

	if (c == spatMode || c == circleRadius || c == circleAngle)
	{
		if (c == spatMode)
		{
			SpatMode sm = spatMode->getValueDataAsEnum<SpatMode>();
			circleRadius->setEnabled(sm == CIRCLE);
			circleAngle->setEnabled(sm == CIRCLE);
		}

		placeTargets();
	}
	else if (c == spatRadius)
	{
		updateRadiuses();
	}
}

void SpatNode::itemAdded(SpatSource* s)
{
	s->setIndex(sources.items.indexOf(s) + 1);
	if (isCurrentlyLoadingData) return;
	ScopedSuspender sp(processor);
	setAudioInputs(sources.items.size());
}

void SpatNode::itemRemoved(SpatSource* s)
{
	if (isCurrentlyLoadingData) return;
	ScopedSuspender sp(processor);
	setAudioInputs(sources.items.size());
}

void SpatNode::itemAdded(SpatTarget* s)
{
	s->setIndex(targets.items.indexOf(s) + 1);
	if (isCurrentlyLoadingData) return;
	ScopedSuspender sp(processor);
	setAudioOutputs(targets.items.size());
}

void SpatNode::itemRemoved(SpatTarget* s)
{
	if (isCurrentlyLoadingData) return;
	ScopedSuspender sp(processor);
	setAudioOutputs(targets.items.size());
}

void SpatNode::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
	int numSamples = buffer.getNumSamples();

	int numInputs = jmin(getNumAudioInputs(), sources.items.size());
	int numOutputs = jmin(getNumAudioOutputs(), targets.items.size());

	AudioSampleBuffer targetBuffer(numOutputs, numSamples);
	targetBuffer.clear();

	for (int i = 0; i < numInputs; i++) processSource(i, buffer, targetBuffer);

	for (int outputIndex = 0; outputIndex < numOutputs; outputIndex++)
	{
		buffer.clear(outputIndex, 0, numSamples);
		buffer.addFrom(outputIndex, 0, targetBuffer.getReadPointer(outputIndex), numSamples);

		jassert(!processor->isSuspended());
		SpatTarget* si = targets.items[outputIndex];

		float rms = buffer.getRMSLevel(outputIndex, 0, buffer.getNumSamples());
		float curVal = si->rms->floatValue();
		float targetVal = si->rms->getLerpValueTo(rms, rms > curVal ? .8f : .2f);
		si->rms->setValue(targetVal); //to change
	}
}

void SpatNode::processSource(int index, AudioBuffer<float>& sourceBuffer, AudioBuffer<float>& targetBuffer)
{
	if (index >= sources.items.size()) return;

	SpatSource* s = sources.items[index];
	if (!s->enabled->boolValue()) return;

	s->update();

	Point<float> pos = s->position->getPoint();
	int numOutputs = targets.items.size();
	int numSamples = sourceBuffer.getNumSamples();

	for (int outputIndex = 0; outputIndex < numOutputs; outputIndex++)
	{
		SpatTarget* si = targets.items[outputIndex];
		float dist = pos.getDistanceFrom(si->position->getPoint());
		float rad = si->radius->floatValue();
		float relDist = jlimit<float>(0, 1, rad == 0 ? 1 : dist / rad);
		float weight = fadeCurve.getValueAtPosition(relDist);

		if (weight > 0) targetBuffer.addFrom(outputIndex, 0, sourceBuffer.getReadPointer(index), numSamples, weight);

		si->weights[index]->setValue(weight);
	}
}

void SpatNode::afterLoadJSONDataInternal()
{
	Node::afterLoadJSONDataInternal();

	setAudioInputs(sources.items.size());
	setAudioOutputs(targets.items.size());
}

BaseNodeViewUI* SpatNode::createViewUI()
{
	return new SpatNodeViewUI(this);
}
