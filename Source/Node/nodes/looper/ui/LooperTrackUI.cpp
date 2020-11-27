/*
  ==============================================================================

    LooperTrackUI.cpp
    Created: 21 Nov 2020 6:33:12pm
    Author:  bkupe

  ==============================================================================
*/

#include "LooperTrackUI.h"
#include "Common/AudioUIHelpers.h"

LooperTrackUI::LooperTrackUI(LooperTrack* t) :
    InspectableContentComponent(t),
    track(t),
    feedback(t)
{
    playRecordUI.reset(track->playRecordTrigger->createButtonUI());
    playRecordUI->customLabel = ">";

    stopUI.reset(track->stopTrigger->createButtonUI());
    clearUI.reset(track->clearTrigger->createButtonUI());

    activeUI.reset(track->active->createButtonToggle());
    volumeUI.reset(track->volume->createSlider());
    volumeUI->orientation = volumeUI->VERTICAL;

    rmsUI.reset(new RMSSliderUI(track->rms));

    addAndMakeVisible(playRecordUI.get());
    addAndMakeVisible(stopUI.get());
    addAndMakeVisible(clearUI.get());
    addAndMakeVisible(activeUI.get());
    activeUI->customLabel = String(t->index + 1);
    activeUI->useCustomFGColor = true;
    activeUI->customFGColor = HIGHLIGHT_COLOR.darker(.25f);

    addAndMakeVisible(volumeUI.get());
    addAndMakeVisible(&feedback);
    addAndMakeVisible(rmsUI.get());
    
    track->addAsyncContainerListener(this);
}

LooperTrackUI::~LooperTrackUI()
{
    if (!inspectable.wasObjectDeleted()) track->removeAsyncContainerListener(this);
}

void LooperTrackUI::paint(Graphics& g)
{
    if (track->isCurrent->boolValue())
    {
        g.setColour(BLUE_COLOR.withAlpha(.2f));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 2);
    }

    g.setColour(feedback.contourColor);
    g.drawRoundedRectangle(getLocalBounds().reduced(1).toFloat(), 2, 1);
}

void LooperTrackUI::resized()
{
    Rectangle<int> r = getLocalBounds().reduced(4);
    //Rectangle<int> vr = r.removeFromRight(30).reduced(1);

    feedback.setBounds(r.removeFromTop(10).reduced(1));
    
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
            feedback.trackStateUpdate(track->trackState->getValueDataAsEnum<LooperTrack::TrackState>());
        }
        else if (e.targetControllable == track->isCurrent)
        {
            repaint();
        }
    }
}


//Feedback
LooperTrackUI::Feedback::Feedback(LooperTrack* t) :
    track(t)
{
}

LooperTrackUI::Feedback::~Feedback()
{
}

void LooperTrackUI::Feedback::trackStateUpdate(LooperTrack::TrackState s)
{
    switch (s)
    {
    case LooperTrack::RECORDING: 
    case LooperTrack::PLAYING:
        startTimerHz(20); 
        break;

    case LooperTrack::STOPPED:
    case LooperTrack::IDLE:
        stopTimer();
        repaint();
        break;

    default:
        repaint();
        break;
    }
}

void LooperTrackUI::Feedback::paint(Graphics& g)
{
    Rectangle<float> r = getLocalBounds().reduced(1).toFloat();

    LooperTrack::TrackState s = track->trackState->getValueDataAsEnum<LooperTrack::TrackState>();
    fillColor = NORMAL_COLOR;
    contourColor = Colours::black;
    switch (s)
    {
    case LooperTrack::IDLE: break;
    case LooperTrack::WILL_RECORD: contourColor = RED_COLOR; break;
    case LooperTrack::RECORDING: 
    {
        float f = cosf(Time::getApproximateMillisecondCounter() / 500.0f) * .5f + .5f;
        fillColor = RED_COLOR.darker(f); break;
    }

    case LooperTrack::FINISH_RECORDING: fillColor = HIGHLIGHT_COLOR; break;
    case LooperTrack::PLAYING: fillColor = GREEN_COLOR; break;
    case LooperTrack::WILL_STOP: fillColor = BLUE_COLOR; break;
    case LooperTrack::STOPPED: fillColor = fillColor.brighter();  break;
    case LooperTrack::WILL_PLAY: contourColor = GREEN_COLOR; break;
    }

    if (contourColor == Colours::black) contourColor = fillColor.brighter(.3f);

    if (track->isPlaying(false))
    {
        g.setColour(NORMAL_COLOR);
        g.fillRoundedRectangle(r, 2);
        float progression = track->loopProgression->floatValue();
        g.setColour(fillColor);
        g.fillRoundedRectangle(r.withWidth(progression * getWidth()), 2);
    }
    else
    {
        g.setColour(fillColor);
        g.fillRoundedRectangle(r, 2);
    }
    
    g.setColour(contourColor);
    g.drawRoundedRectangle(r, 4, 1);
}

void LooperTrackUI::Feedback::timerCallback()
{
    repaint();
}