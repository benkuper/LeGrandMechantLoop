/*
  ==============================================================================

    LooperNodeViewUI.cpp
    Created: 15 Nov 2020 8:43:15am
    Author:  bkupe

  ==============================================================================
*/

#include "LooperNodeViewUI.h"
#include "LooperTrackUI.h"
#include "../MIDILooperNode.h"

LooperNodeViewUI::LooperNodeViewUI(LooperNode * n) :
    NodeViewUI(n)
{
    if (MIDILooperNode * mlp = dynamic_cast<MIDILooperNode*>(node))
    {
        midiParamUI.reset(mlp->midiParam->createMIDIParameterUI());
        addAndMakeVisible(midiParamUI.get());
        contentComponents.add(midiParamUI.get());
    }

    updateTracksUI();
}

LooperNodeViewUI::~LooperNodeViewUI()
{
}

void LooperNodeViewUI::updateTracksUI()
{
    int numTracks = node->tracksCC.controllableContainers.size();

    while (tracksUI.size() > numTracks)
    {
        removeChildComponent(tracksUI[tracksUI.size()-1]);
        tracksUI.removeLast();
    }
    
    while (tracksUI.size() < numTracks)
    {
        LooperTrackUI* ui = new LooperTrackUI((LooperTrack *)node->tracksCC.controllableContainers[tracksUI.size()].get());
        addAndMakeVisible(ui);
        tracksUI.add(ui);
    }

    resized();
}

void LooperNodeViewUI::controllableFeedbackUpdateInternal(Controllable* c)
{
    NodeViewUI::controllableFeedbackUpdateInternal(c);
    if (c == node->numTracks) updateTracksUI();
}

void LooperNodeViewUI::resizedInternalHeader(Rectangle<int>& r)
{
    NodeViewUI::resizedInternalHeader(r);
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


