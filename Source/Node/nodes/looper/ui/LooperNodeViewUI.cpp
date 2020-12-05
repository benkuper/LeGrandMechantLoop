/*
  ==============================================================================

    LooperNodeViewUI.cpp
    Created: 15 Nov 2020 8:43:15am
    Author:  bkupe

  ==============================================================================
*/

#include "LooperNodeViewUI.h"
#include "LooperTrackUI.h"

LooperNodeViewUI::LooperNodeViewUI(GenericNode<LooperProcessor>* n) :
    GenericNodeViewUI(n)
{
    updateTracksUI();
}

LooperNodeViewUI::~LooperNodeViewUI()
{
}

void LooperNodeViewUI::updateTracksUI()
{
    int numTracks = processor->tracksCC.controllableContainers.size();

    while (tracksUI.size() > numTracks)
    {
        removeChildComponent(tracksUI[tracksUI.size()-1]);
        tracksUI.removeLast();
    }
    
    while (tracksUI.size() < numTracks)
    {
        LooperTrackUI* ui = new LooperTrackUI((LooperTrack *)processor->tracksCC.controllableContainers[tracksUI.size()].get());
        addAndMakeVisible(ui);
        tracksUI.add(ui);
    }

    resized();
}

void LooperNodeViewUI::controllableFeedbackUpdateInternal(Controllable* c)
{
    GenericNodeViewUI::controllableFeedbackUpdateInternal(c);
    if (c == processor->numTracks) updateTracksUI();
}

void LooperNodeViewUI::resizedInternalHeader(Rectangle<int>& r)
{
    GenericNodeViewUI::resizedInternalHeader(r);
    if (midiParamUI != nullptr) midiParamUI->setBounds(r.removeFromRight(100).reduced(1));
}

void LooperNodeViewUI::resizedInternalContentNode(Rectangle<int>& r)
{
    if (tracksUI.size() == 0) return;

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


