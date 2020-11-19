/*
  ==============================================================================

    NodeConnector.cpp
    Created: 16 Nov 2020 3:03:26pm
    Author:  bkupe

  ==============================================================================
*/

#include "NodeConnector.h"
#include "../../ui//NodeViewUI.h"

NodeConnector::NodeConnector(NodeViewUI * nodeViewUI, bool isInput) :
    isInput(isInput),
    nodeViewUI(nodeViewUI)
{
    setRepaintsOnMouseActivity(true);
    updateTooltip();
}

NodeConnector::~NodeConnector()
{
}

void NodeConnector::paint(Graphics& g)
{
    bool hasChannels = !(isInput ? nodeViewUI->item->audioInputNames : nodeViewUI->item->audioOutputNames).isEmpty();
    Colour c = hasChannels ? BLUE_COLOR : NORMAL_COLOR;
    g.setColour(c.brighter(isMouseOverOrDragging()?.3f:0));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 2);
}

void NodeConnector::updateTooltip()
{
    if (nodeViewUI == nullptr || nodeViewUI->item == nullptr) return;

    StringArray s = isInput ? nodeViewUI->item->audioInputNames : nodeViewUI->item->audioOutputNames;
    setTooltip(s.joinIntoString("\n"));
}
