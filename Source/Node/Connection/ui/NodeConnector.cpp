/*
  ==============================================================================

	NodeConnector.cpp
	Created: 16 Nov 2020 3:03:26pm
	Author:  bkupe

  ==============================================================================
*/

#include "Node/NodeIncludes.h"

NodeConnector::NodeConnector(BaseNodeViewUI* nodeViewUI, bool isInput, NodeConnection::ConnectionType connectionType) :
	connectionType(connectionType),
	isInput(isInput),
	nodeViewUI(nodeViewUI)
{
	setRepaintsOnMouseActivity(true);
	update();
}

NodeConnector::~NodeConnector()
{
}

void NodeConnector::paint(Graphics& g)
{
	Colour c = NORMAL_COLOR;

	if (connectionType == NodeConnection::AUDIO)
	{
		bool hasChannels = !(isInput ? nodeViewUI->item->audioInputNames : nodeViewUI->item->audioOutputNames).isEmpty();
		if (hasChannels) c = BLUE_COLOR;
	}
	else if (connectionType == NodeConnection::MIDI)
	{
		c = GREEN_COLOR;
	}

	g.setColour(c.brighter(isMouseOverOrDragging() ? .3f : 0));
	g.fillRoundedRectangle(getLocalBounds().toFloat(), 2);
}

void NodeConnector::update()
{
	if (nodeViewUI == nullptr || nodeViewUI->item == nullptr) return;

	StringArray s = isInput ? nodeViewUI->item->audioInputNames : nodeViewUI->item->audioOutputNames;
	setTooltip(s.joinIntoString("\n"));

	repaint();
}

void NodeConnector::mouseDrag(const MouseEvent& e)
{
	//if (isDragAndDropActive()) return;

	//var dragData(new DynamicObject());
	//dragData.getDynamicObject()->setProperty("showMenuIfNoTarget", e.mods.isRightButtonDown());
	//NodeManagerViewUI* mui =  c->startDragging(dragData, this);
}
