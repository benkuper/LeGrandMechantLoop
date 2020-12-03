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
#include "NodeManagerUI.h"

NodeManagerViewUI::NodeManagerViewUI(NodeManager * manager) :
    BaseManagerViewUI(manager->niceName, manager)
{
    addExistingItems(false);

    connectionManagerUI.reset(new NodeConnectionManagerViewUI(this, manager->connectionManager.get()));
    addAndMakeVisible(connectionManagerUI.get(), 0);

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

NodeViewUI* NodeManagerViewUI::createUIForItem(Node* n)
{
    return n->createViewUI();
}

void NodeManagerViewUI::resized()
{
    BaseManagerViewUI::resized();
    connectionManagerUI->setBounds(getLocalBounds());
    connectionManagerUI->resized();
}

void NodeManagerViewUI::mouseDown(const MouseEvent& e)
{
    if (NodeConnector* c = dynamic_cast<NodeConnector*>(e.originalComponent))  connectionManagerUI->startCreateConnection(c);
    else if (NodeConnectionViewUI::Handle* h = dynamic_cast<NodeConnectionViewUI::Handle*>(e.originalComponent))
    {
        NodeConnectionViewUI* cui = dynamic_cast<NodeConnectionViewUI*>(h->getParentComponent());
        NodeConnector* c = h == &cui->sourceHandle ? cui->destConnector : cui->sourceConnector;
        connectionManagerUI->startCreateConnection(c, cui->item);
    }
    else if (e.eventComponent == this) BaseManagerViewUI::mouseDown(e);
}

void NodeManagerViewUI::mouseDrag(const MouseEvent& e)
{
    if (connectionManagerUI->tmpConnectionUI != nullptr) connectionManagerUI->updateCreateConnection();
    else if (e.eventComponent == this) BaseManagerViewUI::mouseDrag(e);
}

void NodeManagerViewUI::mouseUp(const MouseEvent& e)
{
    if (connectionManagerUI->tmpConnectionUI != nullptr) connectionManagerUI->endCreateConnection();
    else if(e.eventComponent == this) BaseManagerViewUI::mouseUp(e);
}

NodeConnector* NodeManagerViewUI::getCandidateConnector(bool lookForInput, NodeConnection::ConnectionType connectionType, NodeViewUI* excludeUI)
{
    for (auto& ui : itemsUI)
    {
        if (ui == excludeUI) continue;
        if (!ui->isVisible()) continue;
        
        NodeConnector* c = nullptr;
        if(connectionType == NodeConnection::AUDIO) c = lookForInput ? ui->inAudioConnector.get() : ui->outAudioConnector.get();
        else if(connectionType == NodeConnection::MIDI) c = lookForInput ? ui->inMIDIConnector.get() : ui->outMIDIConnector.get();

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
            String n = m == RootNodeManager::getInstance() ? "Root" : m->parentContainer->parentContainer->niceName; //container's name

            TextButton * bt = new TextButton(n);
            bt->addListener(this);
            bt->setEnabled(m != managerUI->manager);
            addAndMakeVisible(bt);

            crumbsBT.insert(0,bt);
            crumbManagers.insert(0,m);
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
