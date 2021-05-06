/*
  ==============================================================================

	LooperNodeViewUI.cpp
	Created: 15 Nov 2020 8:43:15am
	Author:  bkupe

  ==============================================================================
*/

LooperNodeViewUI::LooperNodeViewUI(LooperNode* n) :
	NodeViewUI(n)
{
	viewFilterUpdated();
}

LooperNodeViewUI::~LooperNodeViewUI()
{
}

void LooperNodeViewUI::updateTracksUI()
{
	int numTracks = node->showTracks->boolValue() ? node->tracksCC.controllableContainers.size() : 0;

	while (tracksUI.size() > numTracks)
	{
		removeChildComponent(tracksUI[tracksUI.size() - 1]);
		contentComponents.removeAllInstancesOf(tracksUI[tracksUI.size() - 1]);
		tracksUI.removeLast();
	}

	while (tracksUI.size() < numTracks)
	{
		LooperTrackUI* ui = new LooperTrackUI((LooperTrack*)node->tracksCC.controllableContainers[tracksUI.size()].get());
		addAndMakeVisible(ui);
		contentComponents.add(ui);
		tracksUI.add(ui);
	}

	for (auto& t : tracksUI) t->setViewedComponents(node->showTrackRec->boolValue(), node->showTrackStopClear->boolValue(), node->showTrackGains->boolValue(), node->showTrackRMS->boolValue(), node->showTrackActives->boolValue());

	resized();
}

void LooperNodeViewUI::controllableFeedbackUpdateInternal(Controllable* c)
{
	NodeViewUI::controllableFeedbackUpdateInternal(c);
	if (c == node->numTracks) updateTracksUI();
}

void LooperNodeViewUI::viewFilterUpdated()
{
	if (node->showGlobalControl->boolValue())
	{
		if (MIDILooperNode* mlp = dynamic_cast<MIDILooperNode*>(node))
		{
			if (midiParamUI == nullptr)
			{
				midiParamUI.reset(new MIDIDeviceParameterUI(node->midiParam, true, false));
				addAndMakeVisible(midiParamUI.get());
				contentComponents.add(midiParamUI.get());
			}
		}
		
		
		if (recUI == nullptr)
		{
			recUI.reset(node->recTrigger->createButtonUI());
			clearUI.reset(node->clearCurrentTrigger->createButtonUI());

			playAllUI.reset(node->playAllTrigger->createButtonUI());
			stopAllUI.reset(node->stopAllTrigger->createButtonUI());
			clearAllUI.reset(node->clearAllTrigger->createButtonUI());

			addAndMakeVisible(recUI.get());
			addAndMakeVisible(clearUI.get());
			addAndMakeVisible(playAllUI.get());
			addAndMakeVisible(stopAllUI.get());
			addAndMakeVisible(clearAllUI.get());

			contentComponents.add(recUI.get());
			contentComponents.add(clearUI.get());
			contentComponents.add(playAllUI.get());
			contentComponents.add(stopAllUI.get());
			contentComponents.add(clearAllUI.get());
		}
	}
	else
	{
		if (MIDILooperNode* mlp = dynamic_cast<MIDILooperNode*>(node))
		{
			if (midiParamUI != nullptr)
			{
				removeChildComponent(midiParamUI.get());
				contentComponents.removeAllInstancesOf(midiParamUI.get());
				midiParamUI.reset(mlp->midiParam->createMIDIParameterUI());
			}
		}


		if (recUI != nullptr)
		{
			contentComponents.removeAllInstancesOf(recUI.get());
			contentComponents.removeAllInstancesOf(clearUI.get());
			contentComponents.removeAllInstancesOf(playAllUI.get());
			contentComponents.removeAllInstancesOf(stopAllUI.get());
			contentComponents.removeAllInstancesOf(clearAllUI.get());

			removeChildComponent(recUI.get());
			removeChildComponent(clearUI.get());
			removeChildComponent(playAllUI.get());
			removeChildComponent(stopAllUI.get());
			removeChildComponent(clearAllUI.get());

			recUI.reset();
			clearUI.reset();

			playAllUI.reset();
			stopAllUI.reset();
			clearAllUI.reset();
		}
	}
	
	updateTracksUI();

	NodeViewUI::viewFilterUpdated();
}

void LooperNodeViewUI::resizedInternalHeader(Rectangle<int>& r)
{
	NodeViewUI::resizedInternalHeader(r);
	if (midiParamUI != nullptr) midiParamUI->setBounds(r.removeFromRight(100).reduced(1));
}

void LooperNodeViewUI::resizedInternalContentNode(Rectangle<int>& r)
{
	if (tracksUI.size() == 0) return;

	if (recUI != nullptr)
	{
		Rectangle<int> cr = r.removeFromTop(30);
		recUI->setBounds(cr.removeFromLeft(100).reduced(1));
		clearUI->setBounds(cr.removeFromLeft(70).reduced(1));
		clearAllUI->setBounds(cr.removeFromLeft(50).reduced(1));
		cr.setWidth(jmin(cr.getWidth(), 70));
		playAllUI->setBounds(cr.removeFromTop(cr.getHeight() / 2).reduced(1));
		stopAllUI->setBounds(cr.reduced(1));
		r.removeFromTop(1);
	}

	if (node->showTracks->boolValue())
	{
		const int maxTracksPerLine = 8;
		const int trackWidth = jmin<int>(floor((r.getWidth() - 4) / jmax(jmin(maxTracksPerLine, tracksUI.size()), 1)), 40);

		int numLines = ceil(tracksUI.size() * 1.0f / maxTracksPerLine);
		const int lineHeight = r.getHeight() / numLines;

		int index = 0;
		for (int i = 0; i < numLines; i++)
		{
			Rectangle<int> lr = r.removeFromTop(lineHeight).reduced(1);

			for (int j = 0; j < maxTracksPerLine && index < tracksUI.size(); j++)
			{
				tracksUI[index++]->setBounds(lr.removeFromLeft(trackWidth).reduced(1));
			}

			if (index >= tracksUI.size()) break;
		}
	}
}


