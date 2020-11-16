/*
  ==============================================================================

    NodeConnector.cpp
    Created: 16 Nov 2020 3:03:26pm
    Author:  bkupe

  ==============================================================================
*/

#include "NodeConnector.h"
#include "../../Node.h"

NodeConnector::NodeConnector(Node* n, bool isInput) :
    isInput(isInput),
    node(n)
{
    setRepaintsOnMouseActivity(true);
    updateTooltip();
}

NodeConnector::~NodeConnector()
{
}

void NodeConnector::paint(Graphics& g)
{
    g.setColour(BLUE_COLOR.brighter(isMouseOverOrDragging()?.3f:0));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 2);
}

void NodeConnector::updateTooltip()
{
    if (node == nullptr) return;

    StringArray s = isInput ? node->audioInputNames : node->audioOutputNames;
    setTooltip(s.joinIntoString("\n"));
}
