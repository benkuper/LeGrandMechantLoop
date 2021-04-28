/*
  ==============================================================================

    NodeConnectionViewUI.cpp
    Created: 16 Nov 2020 10:01:02am
    Author:  bkupe

  ==============================================================================
*/

#include "Engine/LGMLSettings.h"

NodeConnectionViewUI::NodeConnectionViewUI(NodeConnection* connection, NodeConnector* _sourceConnector, NodeConnector* _destConnector) :
	BaseItemMinimalUI(connection),
	sourceConnector(nullptr),
	destConnector(nullptr),
	activityLevel(0),
	sourceHandle(connection != nullptr ? (connection->connectionType == NodeConnection::AUDIO ? BLUE_COLOR : GREEN_COLOR) : NORMAL_COLOR),
	destHandle(connection != nullptr?(connection->connectionType == NodeConnection::AUDIO?BLUE_COLOR:GREEN_COLOR):NORMAL_COLOR)
{
	setOpaque(false);

	dragAndDropEnabled = false;

	setSourceConnector(_sourceConnector);
	setDestConnector(_destConnector);

	setRepaintsOnMouseActivity(true);
	autoDrawContourWhenSelected = false;

	
	if (connection != nullptr)
	{
		sourceHandle.setSize(16, 16);
		destHandle.setSize(16, 16);
		addAndMakeVisible(&sourceHandle);
		addAndMakeVisible(&destHandle);
	}

	if(LGMLSettings::getInstance()->animateConnectionIntensity->boolValue()) startTimerHz(10);
	if(item != nullptr) item->addAsyncConnectionListener(this);
}

NodeConnectionViewUI::~NodeConnectionViewUI()
{
	if(item != nullptr && !inspectable.wasObjectDeleted()) item->removeAsyncConnectionListener(this);

	setSourceConnector(nullptr);
	setDestConnector(nullptr);
}

void NodeConnectionViewUI::paint(Graphics& g)
{
	NodeAudioConnection* ac = nullptr;
	
	Colour c = YELLOW_COLOR;

	if (item != nullptr)
	{
		NodeConnection::ConnectionType ct = sourceConnector != nullptr ? sourceConnector->connectionType : destConnector->connectionType;
		if (item->connectionType == NodeConnection::AUDIO) ac = (NodeAudioConnection*)item;
		if (item->isSelected) c = HIGHLIGHT_COLOR;
		else
		{
			if (item != nullptr)
			{
				c = ct == NodeConnection::AUDIO ? BLUE_COLOR : GREEN_COLOR;
				c = c.darker().brighter(activityLevel*5);
			}
			if (ac != nullptr && ac->channelMap.size() == 0) c = NORMAL_COLOR;
		}
	}

	if (isMouseOverOrDragging()) c = c.brighter();
	g.setColour(c);
	g.strokePath(path, PathStrokeType(isMouseOverOrDragging() ? 3.0f : jmin<float>(1 + activityLevel * 3, 20)));

	if (item == nullptr) return;

	if (ac != nullptr)
	{
		Rectangle<float> tr = Rectangle<float>(0, 0, 12, 12).withCentre(path.getPointAlongPath(path.getLength() * .5f));
		g.fillEllipse(tr);
		g.setColour(c.darker(.8f));
		g.setFont(g.getCurrentFont().withHeight(12).boldened());
		g.drawText(String(ac->channelMap.size()), tr, Justification::centred);
	}
}

void NodeConnectionViewUI::updateBounds()
{
    if ((sourceConnector == nullptr && destConnector == nullptr) || getParentComponent() == nullptr) return;

    Rectangle<float> mouseR = Rectangle<int>(0, 0, 10, 10).withCentre(getParentComponent()->getMouseXYRelative()).toFloat();
    Rectangle<float> sourceR = sourceConnector == nullptr? mouseR: getParentComponent()->getLocalArea(sourceConnector, sourceConnector->getLocalBounds().toFloat());
    Rectangle<float> destR = destConnector == nullptr ? mouseR : getParentComponent()->getLocalArea(destConnector, destConnector->getLocalBounds().toFloat());
    
    Rectangle<int> r = sourceR.getUnion(destR).expanded(20).toNearestInt();
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

	if (destP.x >= sourceP.x - 20)
	{
		Point<float> controlPoint1 = sourceP.translated(60, 0);
		Point<float> controlPoint2 = destP.translated(-60, 0);
		path.cubicTo(controlPoint1, controlPoint2, destP);
	}
	else
	{
		Point<float> controlPoint1 = sourceP.translated(20, 0);
		Point<float> controlPoint2 = destP.translated(-20, 0);
		float middleY = jmax(sourceP.y, destP.y) - 25;
		path.lineTo(controlPoint1);
		path.lineTo(controlPoint1.x, middleY);
		path.lineTo(controlPoint2.x, middleY);
		path.lineTo(controlPoint2);
		path.lineTo(destP);
		path = path.createPathWithRoundedCorners(10);
	}
	
	hitPath = PathHelpers::buildHitPath(&path);

	if (path.getLength() > 60)
	{
		sourceHandle.setVisible(true);
		destHandle.setVisible(true);
		sourceHandle.setCentrePosition(path.getPointAlongPath(15).toInt());
		destHandle.setCentrePosition(path.getPointAlongPath(path.getLength() - 15).toInt());
	}
	else
	{
		sourceHandle.setVisible(false);
		destHandle.setVisible(false);
	}
	
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

void NodeConnectionViewUI::newMessage(const NodeConnection::ConnectionEvent& e)
{
	if (e.type == e.CHANNELS_CONNECTION_CHANGED) repaint();
}

void NodeConnectionViewUI::timerCallback()
{
	if (item == nullptr)
	{
		stopTimer();
		return;
	}

	if (activityLevel != item->activityLevel)
	{
		activityLevel = item->activityLevel;
		repaint();
	}
}

NodeConnectionViewUI::Handle::Handle(Colour c):
	color(c)
{
	setRepaintsOnMouseActivity(true);
}

void NodeConnectionViewUI::Handle::paint(Graphics& g)
{
	Rectangle<float> r = getLocalBounds().reduced(4).toFloat();
	g.setColour(isMouseOverOrDragging()? color.brighter(.1f):BG_COLOR);
	g.fillEllipse(r);
	
	g.setColour(color.brighter(isMouseOverOrDragging()?.3f:0));
	g.drawEllipse(r, 1);
}
