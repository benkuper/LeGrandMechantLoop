/*
  ==============================================================================

    NodeConnectionViewUI.cpp
    Created: 16 Nov 2020 10:01:02am
    Author:  bkupe

  ==============================================================================
*/

#include "NodeConnectionViewUI.h"
#include "NodeConnector.h"
#include "Common/ConnectionUIHelper.h"

NodeConnectionViewUI::NodeConnectionViewUI(NodeConnection* connection, NodeConnector * _sourceConnector, NodeConnector * _destConnector) :
    BaseItemMinimalUI(connection),
	sourceConnector(nullptr),
	destConnector(nullptr)
{
	setSourceConnector(_sourceConnector);
	setDestConnector(_destConnector);

	setRepaintsOnMouseActivity(true);
	autoDrawContourWhenSelected = false;
}

NodeConnectionViewUI::~NodeConnectionViewUI()
{
	setSourceConnector(nullptr);
	setDestConnector(nullptr);
}

void NodeConnectionViewUI::paint(Graphics& g)
{
	Colour c = item != nullptr ? (item->isSelected ? HIGHLIGHT_COLOR : BLUE_COLOR) : YELLOW_COLOR;
	if (isMouseOverOrDragging()) c = c.brighter();
	g.setColour(c);
	g.strokePath(path, PathStrokeType(isMouseOverOrDragging()?3:1));

	

}

void NodeConnectionViewUI::updateBounds()
{
    if ((sourceConnector == nullptr && destConnector == nullptr) || getParentComponent() == nullptr) return;

    Rectangle<float> mouseR = Rectangle<int>(0, 0, 10, 10).withCentre(getParentComponent()->getMouseXYRelative()).toFloat();
    Rectangle<float> sourceR = sourceConnector == nullptr? mouseR: getParentComponent()->getLocalArea(sourceConnector, sourceConnector->getLocalBounds().toFloat());
    Rectangle<float> destR = destConnector == nullptr ? mouseR : getParentComponent()->getLocalArea(destConnector, destConnector->getLocalBounds().toFloat());
    
    Rectangle<int> r = sourceR.getUnion(destR).expanded(10).toNearestInt();
    setBounds(r); 
    buildPath();
    resized();
}


void NodeConnectionViewUI::buildPath()
{
	Point<float> mouseP = Rectangle<int>(0, 0, 10, 10).withCentre(getMouseXYRelative()).getCentre().toFloat();
	Point<float> sourceP = sourceConnector == nullptr ? mouseP :getLocalPoint(sourceConnector, sourceConnector->getLocalBounds().getCentre()).toFloat();
	Point<float> destP = destConnector == nullptr ? mouseP : getLocalPoint(destConnector, destConnector->getLocalBounds().getCentre()).toFloat();

	path.clear();
	path.startNewSubPath(sourceP);
	path.cubicTo(sourceP.translated(60, 0), destP.translated(-60, 0), destP);

	hitPath = PathHelpers::buildHitPath(&path);
}
bool NodeConnectionViewUI::hitTest(int x, int y)
{
	return hitPath.contains((float)x, (float)y);
}

void NodeConnectionViewUI::setSourceConnector(NodeConnector* c)
{
	if (sourceConnector == c) return;
	if (sourceConnector != nullptr)
	{
		sourceConnector->removeComponentListener(this);
		if(sourceConnector->getParentComponent() != nullptr) sourceConnector->getParentComponent()->removeComponentListener(this);
	}

	sourceConnector = c;

	if (sourceConnector != nullptr)
	{
		sourceConnector->addComponentListener(this);
		sourceConnector->getParentComponent()->addComponentListener(this);
	}

	updateBounds();
}

void NodeConnectionViewUI::setDestConnector(NodeConnector* c)
{
	if (destConnector == c) return;
	if (destConnector != nullptr)
	{
		destConnector->removeComponentListener(this);
		if (destConnector->getParentComponent() != nullptr) destConnector->getParentComponent()->removeComponentListener(this);
	}

	destConnector = c;

	if (destConnector != nullptr)
	{
		destConnector->addComponentListener(this);
		destConnector->getParentComponent()->addComponentListener(this);
	}

	updateBounds();
}

void NodeConnectionViewUI::componentMovedOrResized(Component& c, bool, bool)
{
	updateBounds();
}

void NodeConnectionViewUI::componentBeingDeleted(Component& c)
{
	if (&c == sourceConnector) setSourceConnector(nullptr);
	else if (&c == destConnector) setDestConnector(nullptr);
}
