/*
  ==============================================================================

    AudioUIHelpers.cpp
    Created: 26 Nov 2020 11:35:59am
    Author:  bkupe

  ==============================================================================
*/

#include "AudioUIHelpers.h"

DecibelSliderUI::DecibelSliderUI(DecibelFloatParameter* p) :
    FloatSliderUI(p),
    decibelParam(p)
{
	showLabel = false;
	showValue = true;
	fixedDecimals = 1;
}

DecibelSliderUI::~DecibelSliderUI()
{

}

void DecibelSliderUI::drawBG(Graphics& g)
{
	FloatSliderUI::drawBG(g);

    Rectangle<int> r = getLocalBounds();
    for (int i = 0; i >= -100; i -=6)
    {
		Colour c = NORMAL_COLOR.withAlpha(i == 0 ? .5f : .2f);
        g.setColour(c);

		int tPos = 0;
		if (orientation == HORIZONTAL)
		{
			tPos = DecibelsHelpers::decibelsToValue(i) * r.getWidth();
			g.drawVerticalLine(tPos, 1, r.getHeight() - 1);
		}
		else
		{
			tPos = (1-DecibelsHelpers::decibelsToValue(i)) * r.getHeight();
			g.drawHorizontalLine(tPos, 1, r.getWidth() - 1);
		}
    }
}

String DecibelSliderUI::getValueText() const
{
	return parameter->floatValue() == 0 ? "-inf" : String::formatted("%.1f", decibelParam->decibels);
}


RMSSliderUI::RMSSliderUI(DecibelFloatParameter* p) :
    DecibelSliderUI(p)
{
    updateRate = 10;
    showLabel = false;
    showValue = false;
}

VolumeControlUI::VolumeControlUI(VolumeControl* item, bool showContour, String customActiveLabel) :
	item(item),
	showContour(showContour),
	customActiveLabel(customActiveLabel)
{
	setViewedComponents(true, true, true);
}

void VolumeControlUI::setViewedComponents(bool showGain, bool showRMS, bool showActive)
{
	if (showGain)
	{
		if (gainUI == nullptr)
		{
			gainUI.reset(new DecibelSliderUI(item->gain));
			gainUI->customLabel = item->niceName;
			gainUI->orientation = FloatSliderUI::VERTICAL;
			addAndMakeVisible(gainUI.get());
		}
	}
	else
	{
		if (gainUI != nullptr)
		{
			removeChildComponent(gainUI.get());
			gainUI.reset();
		}
	}

	if (showRMS)
	{
		if (item->rms != nullptr)
		{
			if (rmsUI == nullptr)
			{
				rmsUI.reset(new RMSSliderUI(item->rms));
				rmsUI->orientation = FloatSliderUI::VERTICAL;

				addAndMakeVisible(rmsUI.get());
			}
		}
	}
	else
	{
		if (rmsUI != nullptr)
		{
			removeChildComponent(rmsUI.get());
			rmsUI.reset();
		}
	}

	if (showActive)
	{
		if (activeUI == nullptr)
		{
			activeUI.reset(item->active->createButtonToggle());
			if (customActiveLabel.isNotEmpty()) activeUI->customLabel = customActiveLabel;
			else activeUI->showLabel = true;
			addAndMakeVisible(activeUI.get());
		}
	}
	else
	{
		if (activeUI != nullptr)
		{
			removeChildComponent(activeUI.get());
			activeUI.reset();
		}
	}

	resized();
}

void VolumeControlUI::paint(Graphics& g)
{
	if (showContour)
	{
		g.setColour(NORMAL_COLOR);
		g.drawRoundedRectangle(getLocalBounds().reduced(1).toFloat(), 2, .5f);
	}
}

void VolumeControlUI::resized()
{
	Rectangle<int> r = getLocalBounds();
	if (showContour) r.reduce(2, 2);

	if (activeUI != nullptr) activeUI->setBounds(r.removeFromBottom(r.getWidth()).reduced(1));

	if (rmsUI != nullptr)
	{
		rmsUI->setBounds(r.removeFromRight(6));
		r.removeFromRight(1);
	}
	else
	{
		r.reduce(1, 0);
	}

	if (gainUI != nullptr) gainUI->setBounds(r);
}
