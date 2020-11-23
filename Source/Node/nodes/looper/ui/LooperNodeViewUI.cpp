/*
  ==============================================================================

    LooperNodeViewUI.cpp
    Created: 15 Nov 2020 8:43:15am
    Author:  bkupe

  ==============================================================================
*/

#include "LooperNodeViewUI.h"
#include "LooperTrackUI.h"

LooperNodeViewUI::LooperNodeViewUI(GenericAudioNode<LooperProcessor>* n) :
    GenericAudioNodeViewUI(n)
{
    updateTracksUI();
}

LooperNodeViewUI::~LooperNodeViewUI()
{
}

void LooperNodeViewUI::updateTracksUI()
{
    int numTracks = audioNode->processor->tracksCC.controllableContainers.size();

    while (tracksUI.size() > numTracks)
    {
        removeChildComponent(tracksUI[tracksUI.size()-1]);
        tracksUI.removeLast();
    }
    
    while (tracksUI.size() < numTracks)
    {
        LooperTrackUI* ui = new LooperTrackUI((LooperTrack *)audioNode->processor->tracksCC.controllableContainers[tracksUI.size()].get());
        addAndMakeVisible(ui);
        tracksUI.add(ui);
    }

    resized();
}

void LooperNodeViewUI::controllableFeedbackUpdateInternal(Controllable* c)
{
    GenericAudioNodeViewUI::controllableFeedbackUpdateInternal(c);
    if (c == audioNode->processor->numTracks) updateTracksUI();
}

void LooperNodeViewUI::resizedInternalContentNode(Rectangle<int>& r)
{
    const int maxTracksPerLine = 16;
    const int trackWidth = jmin<int>(floor((r.getWidth()-4)/jmax(jmin(maxTracksPerLine, tracksUI.size()),1)), 40);
    
    int numLines = ceil(tracksUI.size() * 1.0f / maxTracksPerLine);
    const int lineHeight = r.getHeight() / numLines;

    int index = 0;
    for (int i = 0; i < numLines; i++)
    {
        Rectangle<int> lr = r.removeFromTop(lineHeight).reduced(2);

        for (int j = 0; j < maxTracksPerLine && index < tracksUI.size(); j++)
        {
            tracksUI[index++]->setBounds(lr.removeFromLeft(trackWidth).reduced(1));
        }

        if (index >= tracksUI.size()) break;
    }
}


