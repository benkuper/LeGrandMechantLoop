/*
  ==============================================================================

	TimelineNodeUI.cpp
	Created: 26 May 2025 6:42:33pm
	Author:  bkupe

  ==============================================================================
*/

#include "Node/NodeIncludes.h"
#include "TimelineNodeUI.h"

inline TimelineNodeUI::TimelineNodeUI(TimelineNode* node) :
	NodeViewUI(node)
{
	sequenceManagerUI.reset(new SequenceManagerUI("Sequences", &node->sequenceManager));
	addAndMakeVisible(sequenceManagerUI.get());
}

void TimelineNodeUI::resizedInternalContentNode(Rectangle<int>& r)
{
	sequenceManagerUI->setBounds(r);
}
