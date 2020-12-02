/*
  ==============================================================================

    LooperTrackUI.h
    Created: 21 Nov 2020 6:33:12pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "../LooperTrack.h"

class LooperTrackUI :
    public InspectableContentComponent,
    public ContainerAsyncListener
{
public:
    LooperTrackUI(LooperTrack * t);
    ~LooperTrackUI();

    LooperTrack* track;

    std::unique_ptr<TriggerButtonUI> playRecordUI;
    std::unique_ptr<TriggerButtonUI> stopUI;
    std::unique_ptr<TriggerButtonUI> clearUI;

    std::unique_ptr<BoolButtonToggleUI> activeUI;
    std::unique_ptr<FloatSliderUI> volumeUI;
    std::unique_ptr<FloatSliderUI> rmsUI;

    class Feedback :
        public Component,
        public Timer
    {
    public:
        Feedback(LooperTrack* t);
        ~Feedback();

        LooperTrack* track;

        Colour contourColor;
        Colour fillColor;


        void trackStateUpdate(LooperTrack::TrackState s);

        void updateContourColor();

        void paint(Graphics& g) override;
        void timerCallback() override;
    };

    Feedback feedback;

    void paint(Graphics& g) override;
    void resized() override;

    void newMessage(const ContainerAsyncEvent& e) override;
};