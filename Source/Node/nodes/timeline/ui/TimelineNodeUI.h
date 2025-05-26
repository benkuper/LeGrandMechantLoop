/*
  ==============================================================================

    TimelineNodeUI.h
    Created: 26 May 2025 6:42:33pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once


class TimelineNodeUI :
    public NodeViewUI<TimelineNode>
{
public:
    TimelineNodeUI(TimelineNode* node);
    ~TimelineNodeUI() {}

    std::unique_ptr<SequenceManagerUI> sequenceManagerUI;

	void resizedInternalContentNode(Rectangle<int> &r) override;
};