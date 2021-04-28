/*
  ==============================================================================

    LooperTrackUI.cpp
    Created: 21 Nov 2020 6:33:12pm
    Author:  bkupe

  ==============================================================================
*/

LooperTrackUI::LooperTrackUI(LooperTrack* t) :
    InspectableContentComponent(t),
    track(t),
    feedback(t)
{
    addAndMakeVisible(&feedback);
    track->addAsyncContainerListener(this);
}

LooperTrackUI::~LooperTrackUI()
{
    if (!inspectable.wasObjectDeleted()) track->removeAsyncContainerListener(this);
}

void LooperTrackUI::paint(Graphics& g)
{
    if (inspectable.wasObjectDeleted()) return;

    if (track->isCurrent->boolValue())
    {
        g.setColour(BLUE_COLOR.withAlpha(.2f));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 2);
    }

    g.setColour(feedback.contourColor);
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1), 2, .5);

}

void LooperTrackUI::resized()
{
    Rectangle<int> r = getLocalBounds().reduced(2);
    //Rectangle<int> vr = r.removeFromRight(30).reduced(1);

    feedback.setBounds(r.removeFromTop(10).reduced(1));
    
    if (playRecordUI != nullptr)
    {
        playRecordUI->setBounds(r.removeFromTop(30).reduced(1));
    }

    if (stopUI != nullptr)
    {
        stopUI->setBounds(r.removeFromTop(20).reduced(1));
        clearUI->setBounds(r.removeFromTop(20).reduced(1));
    }
    
    if(volumeUI != nullptr) volumeUI->setBounds(r.reduced(1));
}

void LooperTrackUI::setViewedComponents(bool showRec, bool showStopClear, bool showGains, bool showRMS, bool showActives)
{
    if (showRec)
    {
        if (playRecordUI == nullptr)
        {
            playRecordUI.reset(track->playRecordTrigger->createButtonUI());
            Colour c = Colour::fromHSV((track->section->intValue() - 1) * .2f, .3f, .6f, 1);
            playRecordUI->customLabel = track->section->stringValue();
            playRecordUI->customBGColor = c;
            playRecordUI->useCustomBGColor = true;
            addAndMakeVisible(playRecordUI.get());
        }
    }
    else
    {
        if (playRecordUI != nullptr)
        {
            removeChildComponent(playRecordUI.get());
            playRecordUI.reset();
        }
    }

    if(showStopClear)
    {
        if(stopUI == nullptr)
        {
            stopUI.reset(track->stopTrigger->createButtonUI());
            clearUI.reset(track->clearTrigger->createButtonUI());
            addAndMakeVisible(stopUI.get());
            addAndMakeVisible(clearUI.get());
        }
    }
    else
    {
        if (stopUI != nullptr)
        {
            removeChildComponent(stopUI.get());
            removeChildComponent(clearUI.get());
            stopUI.reset();
            clearUI.reset();
        }
    }
    
    if (showGains || showRMS || showActives)
    {
        if (volumeUI == nullptr)
        {
            volumeUI.reset(new VolumeControlUI(track, false, String(track->index + 1)));
            volumeUI->activeUI->useCustomFGColor = true;
            volumeUI->activeUI->customFGColor = HIGHLIGHT_COLOR.darker(.25f);
            addAndMakeVisible(volumeUI.get());
        }

        volumeUI->setViewedComponents(showGains, showRMS, showActives);
    }
    else
    {
        if (volumeUI != nullptr)
        {
            removeChildComponent(volumeUI.get());
            volumeUI.reset();
        }
    }
   
    resized();
}

void LooperTrackUI::newMessage(const ContainerAsyncEvent& e)
{
    if (e.type == e.ControllableFeedbackUpdate)
    {
        if (e.targetControllable.wasObjectDeleted()) return;
        if (e.targetControllable == track->trackState)
        {
            feedback.trackStateUpdate(track->trackState->getValueDataAsEnum<LooperTrack::TrackState>());
            repaint();
        }
        else if (e.targetControllable == track->isCurrent)
        {
            repaint();
        }
        else if (e.targetControllable == track->section)
        {
            if (playRecordUI != nullptr)
            {
                Colour c = Colour::fromHSV((track->section->intValue() - 1) * .2f, .3f, .6f, 1);
                playRecordUI->customLabel = track->section->stringValue();
                playRecordUI->customBGColor = c;
                playRecordUI->updateUIParams();
                playRecordUI->repaint();
            }
        }
    }
}


//Feedback
LooperTrackUI::Feedback::Feedback(LooperTrack* t) :
    track(t)
{
    setInterceptsMouseClicks(false, false);
    updateContourColor();
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
        break;

    default:
        break;
    }
   
    updateContourColor();
    repaint();
}

void LooperTrackUI::Feedback::updateContourColor()
{
    LooperTrack::TrackState s = track->trackState->getValueDataAsEnum<LooperTrack::TrackState>();
    switch (s)
    {
    case LooperTrack::IDLE: contourColor = NORMAL_COLOR; break;
    case LooperTrack::WILL_RECORD:
    case LooperTrack::RECORDING:
    case LooperTrack::FINISH_RECORDING:
        contourColor = RED_COLOR;
        break;

    case LooperTrack::WILL_PLAY:
    case LooperTrack::PLAYING:
        contourColor = GREEN_COLOR;
        break;

    case LooperTrack::WILL_STOP:
        contourColor = BLUE_COLOR;
        break;

    case LooperTrack::STOPPED:
        contourColor = NORMAL_COLOR.brighter();
        break;
        
    default:
        break;
    }

}

void LooperTrackUI::Feedback::paint(Graphics& g)
{
    Rectangle<float> r = getLocalBounds().reduced(1).toFloat();

    LooperTrack::TrackState s = track->trackState->getValueDataAsEnum<LooperTrack::TrackState>();
    fillColor = NORMAL_COLOR;
    switch (s)
    {
    case LooperTrack::RECORDING:
    {
        float f = cosf(Time::getApproximateMillisecondCounter() / 500.0f) * .5f + .5f;
        fillColor = RED_COLOR.darker(f); break;
    }
    case LooperTrack::FINISH_RECORDING: fillColor = HIGHLIGHT_COLOR; break;
    case LooperTrack::PLAYING: fillColor = GREEN_COLOR; break;
    case LooperTrack::WILL_STOP: fillColor = BLUE_COLOR; break;
    case LooperTrack::STOPPED: fillColor = fillColor.brighter();  break;
        default:
            break;
    }

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
