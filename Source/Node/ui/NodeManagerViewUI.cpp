/*
  ==============================================================================

    NodeManagerViewUI.cpp
    Created: 15 Nov 2020 8:40:22am
    Author:  bkupe

  ==============================================================================
*/

#include "NodeManagerViewUI.h"
#include "../Connection/ui/NodeConnectionManagerViewUI.h"
#include "../Connection/ui/NodeConnector.h"

NodeManagerViewUI::NodeManagerViewUI(StringRef name) :
    BaseManagerShapeShifterViewUI(name, RootNodeManager::getInstance())
{
    addExistingItems(false);

    connectionManagerUI.reset(new NodeConnectionManagerViewUI(this, manager->connectionManager.get()));
    addAndMakeVisible(connectionManagerUI.get(),0);

    enableSnapping = true;

    updatePositionOnDragMove = true;

    removeMouseListener(this);
    addMouseListener(this, true);
    setDisableDefaultMouseEvents(true);
}

NodeManagerViewUI::~NodeManagerViewUI()
{
}

NodeViewUI* NodeManagerViewUI::createUIForItem(Node* n)
{
    return n->createViewUI();
}

void NodeManagerViewUI::resized()
{
    BaseManagerShapeShifterViewUI::resized();
    connectionManagerUI->setBounds(getLocalBounds());
}

void NodeManagerViewUI::mouseDown(const MouseEvent& e)
{
    if (NodeConnector* c = dynamic_cast<NodeConnector*>(e.originalComponent))  connectionManagerUI->startCreateConnection(c);
    else if (e.eventComponent == this) BaseManagerViewUI::mouseDown(e);
}

void NodeManagerViewUI::mouseDrag(const MouseEvent& e)
{
    if (NodeConnector* c = dynamic_cast<NodeConnector*>(e.originalComponent))  connectionManagerUI->updateCreateConnection();
    else if (e.eventComponent == this) BaseManagerViewUI::mouseDrag(e);
}

void NodeManagerViewUI::mouseUp(const MouseEvent& e)
{
    if (NodeConnector* c = dynamic_cast<NodeConnector*>(e.originalComponent))  connectionManagerUI->endCreateConnection();
    else if(e.eventComponent == this) BaseManagerViewUI::mouseUp(e);
}

NodeConnector* NodeManagerViewUI::getCandidateConnector(bool lookForInput, NodeViewUI* excludeUI)
{
    for (auto& ui : itemsUI)
    {
        if (ui == excludeUI) continue;
        if (!ui->isVisible()) continue;
        
        NodeConnector* c = lookForInput ? ui->inAudioConnector.get() : ui->outAudioConnector.get();
        if (c == nullptr) continue;

        if (c->getLocalBounds().expanded(10).contains(c->getMouseXYRelative())) return c;
    }

    return nullptr;
}
