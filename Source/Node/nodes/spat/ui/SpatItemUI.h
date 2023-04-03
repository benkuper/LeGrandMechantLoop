/*
  ==============================================================================

	SpatItemUI.h
	Created: 21 Nov 2020 6:32:53pm
	Author:  bkupe

  ==============================================================================
*/

#pragma once

class SpatItemUI :
	public BaseItemMinimalUI<SpatItem>
{
public:
	SpatItemUI(SpatItem* item);
	virtual ~SpatItemUI();

	SpatItem* item;
	Point<float> posAtMouseDown;

	void mouseDown(const MouseEvent& e) override;
	bool hitTest(int x, int y) override;
	virtual void paint(Graphics& g) override;
};

class SpatSourceUI :
	public SpatItemUI
{
public:
	SpatSourceUI(SpatSource* s) : SpatItemUI(s), source(s) {}
	~SpatSourceUI() {}

	SpatSource* source;
};

class SpatTargetUI :
	public SpatItemUI
{
public:
	SpatTargetUI(SpatTarget* t) : SpatItemUI(t), target(t) {}
	~SpatTargetUI() {}

	SpatTarget* target;

	void paint(Graphics& g) override;

	void controllableFeedbackUpdateInternal(Controllable* c) override;
};