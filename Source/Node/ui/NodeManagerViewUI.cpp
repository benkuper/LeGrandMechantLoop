/*
  ==============================================================================

	NodeManagerViewUI.cpp
	Created: 15 Nov 2020 8:40:22am
	Author:  bkupe

  ==============================================================================
*/

#include "Node/NodeIncludes.h"

NodeManagerViewUI::NodeManagerViewUI(NodeManager* manager) :
	BaseManagerViewUI(manager->niceName, manager),
	draggingConnector(nullptr)
{
	addExistingItems(false);

	connectionManagerUI.reset(new NodeConnectionManagerViewUI(this, manager->connectionManager.get()));
	addAndMakeVisible(connectionManagerUI.get(), 0);

	setShowPane(true);

	bringToFrontOnSelect = false;
	enableSnapping = true;

	updatePositionOnDragMove = true;

	removeMouseListener(this);
	addMouseListener(this, true);
	setDisableDefaultMouseEvents(true);
}

NodeManagerViewUI::~NodeManagerViewUI()
{
}

BaseNodeViewUI* NodeManagerViewUI::createUIForItem(Node* n)
{
	return n->createViewUI();
}

void NodeManagerViewUI::resized()
{
	BaseManagerViewUI::resized();
	connectionManagerUI->setBounds(getLocalBounds());
	connectionManagerUI->resized();
}

void NodeManagerViewUI::addItemUIInternal(BaseNodeViewUI* ui)
{
	BaseManagerViewUI::addItemUIInternal(ui);

	if (connectionManagerUI != nullptr)
	{
		Array<NodeConnection*> connections = manager->connectionManager->getInAndOutConnectionsFor(ui->item);
		for (auto& c : connections)
		{
			if (NodeConnectionViewUI* cui = connectionManagerUI->getUIForItem(c))
			{
				if (c->sourceNode == ui->item && cui->sourceConnector == nullptr) cui->setSourceConnector(cui->item->connectionType == NodeConnection::AUDIO ? ui->outAudioConnector.get() : ui->outMIDIConnector.get());

				if (c->destNode == ui->item && cui->destConnector == nullptr) cui->setDestConnector(cui->item->connectionType == NodeConnection::AUDIO ? ui->inAudioConnector.get() : ui->inMIDIConnector.get());
			}
		}
	}
}

void NodeManagerViewUI::mouseDown(const MouseEvent& e)
{
	BaseManagerViewUI::mouseDown(e);
}

void NodeManagerViewUI::mouseDrag(const MouseEvent& e)
{
	if (NodeConnector* c = dynamic_cast<NodeConnector*>(e.originalComponent))
	{
		if (!isDragAndDropActive())
		{
			var dragData(new DynamicObject());
			dragData.getDynamicObject()->setProperty("showMenuIfNoTarget", e.mods.isRightButtonDown());
			draggingConnector = c;
			forcedDragTargetPos = {};
			startDragging(dragData, c, ScaledImage(Image(Image::RGB, 1, 1, true)));
		}
		return;
	}

	BaseManagerViewUI::mouseDrag(e);
}

void NodeManagerViewUI::mouseUp(const MouseEvent& e)
{
	draggingConnector = nullptr;
	forcedDragTargetPos = {};
	repaint();
	BaseManagerViewUI::mouseUp(e);
}

void NodeManagerViewUI::paintOverChildren(Graphics& g)
{
	BaseManagerViewUI::paintOverChildren(g);

	if (draggingConnector != nullptr)
	{
		g.setColour(draggingConnector->connectionType == NodeConnection::AUDIO ? BLUE_COLOR : GREEN_COLOR);

		Point<float> p1 = getLocalPoint(draggingConnector, draggingConnector->getLocalBounds().getCentre()).toFloat();
		Point<float> p2 = forcedDragTargetPos.isOrigin() ? getMouseXYRelative().toFloat() : forcedDragTargetPos.toFloat();
		Point<float> midP = (p1 + p2) / 2;

		Path p;
		p.startNewSubPath(p1);
		p.cubicTo({ midP.x,p1.y }, { midP.x,p2.y }, p2);
		g.strokePath(p, PathStrokeType(2));

	}
}

bool NodeManagerViewUI::isInterestedInDragSource(const SourceDetails& details)
{
	if (NodeConnector* nc = dynamic_cast<NodeConnector*>(details.sourceComponent.get())) return true;
	return BaseManagerViewUI::isInterestedInDragSource(details);
}

void NodeManagerViewUI::itemDragEnter(const DragAndDropTarget::SourceDetails& details)
{
	if (NodeConnector* nc = dynamic_cast<NodeConnector*>(details.sourceComponent.get()))
	{
		repaint();
		return;
	}
	BaseManagerViewUI::itemDragEnter(details);
}

void NodeManagerViewUI::itemDragMove(const DragAndDropTarget::SourceDetails& details)
{
	if (NodeConnector* nc = dynamic_cast<NodeConnector*>(details.sourceComponent.get()))
	{
		repaint();
		return;
	}
	BaseManagerViewUI::itemDragMove(details);
}

void NodeManagerViewUI::itemDragExit(const DragAndDropTarget::SourceDetails& details)
{

	BaseManagerViewUI::itemDragExit(details);
}

void NodeManagerViewUI::itemDropped(const DragAndDropTarget::SourceDetails& details)
{
	draggingConnector = nullptr;

	if (NodeConnector* nc = dynamic_cast<NodeConnector*>(details.sourceComponent.get()))
	{
		bool showMenu = KeyPress::isKeyCurrentlyDown(16);
		showMenu |= (bool)(int)details.description.getDynamicObject()->getProperty("showMenuIfNoTarget");

		if (showMenu)
		{
			Node* initNode = nc->nodeViewUI->item;
			bool isInput = nc->isInput;
			NodeConnection::ConnectionType connectionType = nc->connectionType;

			showMenuAndAddItem(false, getMouseXYRelative(), [this, initNode, isInput, connectionType](Node* n)
				{
					Node* s = isInput ? n : initNode;
					Node* d = isInput ? initNode : n;
					if (s != nullptr && d != nullptr) manager->connectionManager->addConnection(s, d, connectionType);
				}
			);
		}

		repaint();
		return;
	}

	BaseManagerViewUI::itemDropped(details);

}

void NodeManagerViewUI::dragOperationEnded(const DragAndDropTarget::SourceDetails& details)
{
	draggingConnector = nullptr;
	forcedDragTargetPos = {};
	repaint();
}

NodeConnector* NodeManagerViewUI::getCandidateConnector(bool lookForInput, NodeConnection::ConnectionType connectionType, BaseNodeViewUI* excludeUI)
{
	for (auto& ui : itemsUI)
	{
		if (ui == excludeUI) continue;
		if (!ui->isVisible()) continue;

		NodeConnector* c = nullptr;
		if (connectionType == NodeConnection::AUDIO) c = lookForInput ? ui->inAudioConnector.get() : ui->outAudioConnector.get();
		else if (connectionType == NodeConnection::MIDI) c = lookForInput ? ui->inMIDIConnector.get() : ui->outMIDIConnector.get();

		if (c == nullptr || c->connectionType != connectionType) continue;
		if (c->getLocalBounds().expanded(10).contains(c->getMouseXYRelative())) return c;
	}

	return nullptr;
}

NodeManagerViewPanel::NodeManagerViewPanel(StringRef contentName) :
	ShapeShifterContentComponent(contentName)
{
	contentIsFlexible = true;
	setManager(RootNodeManager::getInstance());
}

NodeManagerViewPanel::~NodeManagerViewPanel()
{
}

void NodeManagerViewPanel::setManager(NodeManager* manager)
{
	if (managerUI != nullptr)
	{
		if (managerUI->manager == manager) return;
		removeChildComponent(managerUI.get());
	}

	if (manager != nullptr) managerUI.reset(new NodeManagerViewUI(manager));
	else managerUI.reset();

	if (managerUI != nullptr)
	{
		addAndMakeVisible(managerUI.get());
	}

	updateCrumbs();
	resized();

	if (NodeManagerPanel* p = ShapeShifterManager::getInstance()->getContentForType<NodeManagerPanel>())
	{
		p->setManager(manager);
	}
}

void NodeManagerViewPanel::updateCrumbs()
{
	for (auto& c : crumbsBT) removeChildComponent(c);
	crumbsBT.clear();
	crumbManagers.clear();

	if (managerUI == nullptr || managerUI->manager == nullptr) return;

	ControllableContainer* pc = managerUI->manager;
	while (pc != nullptr)
	{
		if (NodeManager* m = dynamic_cast<NodeManager*>(pc))
		{
			String n = m == RootNodeManager::getInstance() ? "Root" : m->parentContainer->niceName; //container's name

			TextButton* bt = new TextButton(n);
			bt->addListener(this);
			bt->setEnabled(m != managerUI->manager);
			addAndMakeVisible(bt);

			crumbsBT.insert(0, bt);
			crumbManagers.insert(0, m);
		}

		pc = pc->parentContainer;
	}
}

void NodeManagerViewPanel::resized()
{
	if (managerUI != nullptr) managerUI->setBounds(getLocalBounds());
	Rectangle<int> hr = getLocalBounds().removeFromTop(24);
	hr.removeFromRight(30);
	for (auto& c : crumbsBT) c->setBounds(hr.removeFromLeft(80).reduced(2));
}

void NodeManagerViewPanel::buttonClicked(Button* b)
{
	if (crumbsBT.contains((TextButton*)b))
	{
		setManager(crumbManagers[crumbsBT.indexOf((TextButton*)b)]);
	}
}
