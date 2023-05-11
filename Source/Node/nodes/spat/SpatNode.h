/*
  ==============================================================================

	SpatNode.h
	Created: 15 Nov 2020 8:42:35am
	Author:  bkupe

  ==============================================================================
*/

#pragma once

class SpatNode :
	public Node,
	public BaseManager<SpatSource>::ManagerListener,
	public BaseManager<SpatTarget>::ManagerListener
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
	FloatParameter* circleArc;
	BoolParameter* firstIsCentered;

	BaseManager<SpatSource> sources;
	BaseManager<SpatTarget> targets;

	Array<float, CriticalSection> weights;

	void placeTargets();
	void updateRadiuses();
	void updateWeights();

	void updateAudioInputsInternal() override;

	void onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c) override;
	void itemAdded(SpatSource* s) override;
	void itemRemoved(SpatSource* s) override;
	void itemAdded(SpatTarget* s) override;
	void itemRemoved(SpatTarget* s) override;

	void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;
	void processSource(int index, AudioBuffer<float>& sourceBuffer, AudioBuffer<float>& targetBuffer);


	void afterLoadJSONDataInternal() override;


	DECLARE_TYPE("Spatializer");

	BaseNodeViewUI* createViewUI() override;
};
