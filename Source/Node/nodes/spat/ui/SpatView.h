/*
  ==============================================================================

	SpatView.h
	Created: 21 Nov 2020 6:32:59pm
	Author:  bkupe

  ==============================================================================
*/

#pragma once

class SpatView :
	public Component
{
public:
	SpatView(SpatNode* item);
	~SpatView();

	SpatNode* node;

	OwnedArray<SpatSourceUI> sourcesUI;
	OwnedArray<SpatTargetUI> targetsUI;
	Point<float> posAtMouseDown;

	void paint(Graphics& g) override;
	void resized() override;

	void updateItemsUI();
	void placeTarget(SpatTargetUI* ui);
	void placeSource(SpatSourceUI* ui);
	SpatTargetUI* getUIForTarget(SpatTarget* item);
	SpatSourceUI* getUIForSource(SpatSource* source);

	void mouseDown(const MouseEvent& e) override;
	void mouseDrag(const MouseEvent& e) override;
	void mouseUp(const MouseEvent& e) override;
};