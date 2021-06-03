/*
  ==============================================================================

    FFTAnalyzer.h
    Created: 14 May 2019 4:08:08pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

class FFTAnalyzer :
	public BaseItem
{
public:
	FFTAnalyzer();
	~FFTAnalyzer();
	
	
	FloatParameter* position;
	FloatParameter* size;
	FloatParameter * value;
	ColorParameter * color;

	void process(float * fftSamples, int numSamples);

	void onContainerNiceNameChanged() override;

	InspectableEditor* getEditor(bool isRoot) override	;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FFTAnalyzer)

};
