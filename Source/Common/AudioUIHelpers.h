/*
  ==============================================================================

    AudioUIHelpers.h
    Created: 26 Nov 2020 11:35:59am
    Author:  bkupe

  ==============================================================================
*/

#pragma once
#include "JuceHeader.h"
#include "Node/Node.h"

class DecibelSliderUI :
    public FloatSliderUI
{
public:
    DecibelSliderUI(DecibelFloatParameter* p);
    ~DecibelSliderUI();

    DecibelFloatParameter* decibelParam;

    virtual void drawBG(Graphics& g) override;
    virtual String getValueText() const override;

};

class RMSSliderUI :
    public DecibelSliderUI
{
public:
    RMSSliderUI(DecibelFloatParameter * p);
    ~RMSSliderUI() {}
};


class VolumeControlUI :
    public Component
{
public:
    VolumeControlUI(VolumeControl* item, bool showContour = true, String customActiveLabel = "");
    ~VolumeControlUI() {}

    VolumeControl* item;
    bool showContour;
    String customActiveLabel;

    std::unique_ptr<DecibelSliderUI> gainUI;
    std::unique_ptr<RMSSliderUI> rmsUI;
    std::unique_ptr<BoolButtonToggleUI> activeUI;

    void setViewedComponents(bool showGain, bool showRMS, bool showActive);

    void paint(Graphics& g) override;
    void resized() override;
};