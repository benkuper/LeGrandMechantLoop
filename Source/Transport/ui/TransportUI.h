/*
  ==============================================================================

    TransportUI.h
    Created: 15 Nov 2020 8:53:53am
    Author:  bkupe

  ==============================================================================
*/

#pragma once
#include "../Transport.h"

class TransportUI :
    public ShapeShifterContentComponent,
    public InspectableContent,
    public ContainerAsyncListener
{
public:
    TransportUI(StringRef name);
    ~TransportUI();
    
    Transport* transport;

    std::unique_ptr<TriggerImageUI> playBT;
    std::unique_ptr<TriggerImageUI> stopBT;
    
    std::unique_ptr<IntParameterLabelUI> beatsPerBarUI;
    std::unique_ptr<IntParameterLabelUI> beatUnitUI;
    std::unique_ptr<FloatParameterLabelUI> bpmUI;
    std::unique_ptr<EnumParameterUI> quantizUI;

    std::unique_ptr<IntParameterLabelUI> curBarUI;
    std::unique_ptr<IntParameterLabelUI> curBeatUI;

    class BeatViz :
        public Component
    {
    public:
        BeatViz(int index);
        
        int index;

        Transport* transport;
        void paint(Graphics& g) override;
    };

    class BarViz :
        public Component,
        public Timer
    {
    public:
        BarViz();

        Transport* transport;
        OwnedArray<BeatViz> beats;

        void rebuild();

        void paint(Graphics& g) override;
        void resized() override;

        void timerCallback() override;
    };

    BarViz barViz;

    void resized() override;

    void mouseDown(const MouseEvent& e) override;

    void newMessage(const ContainerAsyncEvent& e) override;

    static TransportUI* create(const String& name) { return new TransportUI(name); }
};