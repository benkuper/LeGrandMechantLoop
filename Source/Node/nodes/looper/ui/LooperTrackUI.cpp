/*
  ==============================================================================

    LooperTrackUI.cpp
    Created: 21 Nov 2020 6:33:12pm
    Author:  bkupe

  ==============================================================================
*/

#include "LooperTrackUI.h"

LooperTrackUI::LooperTrackUI(LooperTrack* t) :
    InspectableContentComponent(t),
    track(t)
{
    playRecordUI.reset(track->playRecordTrigger->createButtonUI());
    playRecordUI->customLabel = ">";

    stopUI.reset(track->stopTrigger->createButtonUI());
    clearUI.reset(track->clearTrigger->createButtonUI());

    activeUI.reset(track->active->createButtonToggle());
    volumeUI.reset(track->volume->createSlider());
    volumeUI->orientation = volumeUI->VERTICAL;

    rmsUI.reset(track->rms->createSlider());
    rmsUI->orientation = rmsUI->VERTICAL;
    rmsUI->showLabel = false;
    rmsUI->showValue = false;

    addAndMakeVisible(playRecordUI.get());
    addAndMakeVisible(stopUI.get());
    addAndMakeVisible(clearUI.get());
    addAndMakeVisible(activeUI.get());
    activeUI->customLabel = String(t->index + 1);
    activeUI->useCustomFGColor = true;
    activeUI->customFGColor = HIGHLIGHT_COLOR.darker(.25f);

    addAndMakeVisible(volumeUI.get());


    addAndMakeVisible(rmsUI.get());
    
    track->addAsyncContainerListener(this);
}

LooperTrackUI::~LooperTrackUI()
{
    if (!inspectable.wasObjectDeleted()) track->removeAsyncContainerListener(this);
}

void LooperTrackUI::paint(Graphics& g)
{

    LooperTrack::TrackState s = track->trackState->getValueDataAsEnum<LooperTrack::TrackState>();
    Colour c = NORMAL_COLOR;
    Colour oc;

    switch (s)
    {
    case LooperTrack::IDLE: ; break;
    case LooperTrack::WILL_RECORD: oc = RED_COLOR; break;
    case LooperTrack::RECORDING: c = RED_COLOR; break;
    case LooperTrack::FINISH_RECORDING: c = HIGHLIGHT_COLOR;  break;
    case LooperTrack::PLAYING: c = GREEN_COLOR; break;
    case LooperTrack::WILL_STOP: c = BLUE_COLOR; break;
    case LooperTrack::STOPPED: c = c.brighter();  break;
    case LooperTrack::WILL_PLAY: oc = GREEN_COLOR; break;
    }

    if(oc == Colours::black) oc = c.brighter(.3f);


    g.setColour(c);
    g.drawRoundedRectangle(getLocalBounds().reduced(1).toFloat(), 2, 1);

    g.setColour(c);
    g.fillRoundedRectangle(statusRect.toFloat(),4);
    g.setColour(oc);
    g.drawRoundedRectangle(statusRect.toFloat(), 4,1);
}

void LooperTrackUI::resized()
{
    Rectangle<int> r = getLocalBounds().reduced(4);
    //Rectangle<int> vr = r.removeFromRight(30).reduced(1);

    statusRect = r.removeFromTop(10).reduced(2);
    
    playRecordUI->setBounds(r.removeFromTop(30).reduced(1));
    stopUI->setBounds(r.removeFromTop(20).reduced(1));
    clearUI->setBounds(r.removeFromTop(20).reduced(1));

    activeUI->setBounds(r.removeFromBottom(r.getWidth()).reduced(1));
    rmsUI->setBounds(r.removeFromRight(8).reduced(1));
    volumeUI->setBounds(r.reduced(1));
}

void LooperTrackUI::newMessage(const ContainerAsyncEvent& e)
{
    if (e.type == e.ControllableFeedbackUpdate)
    {
        if (e.targetControllable == track->trackState)
        {
            repaint();
        }
    }
}
