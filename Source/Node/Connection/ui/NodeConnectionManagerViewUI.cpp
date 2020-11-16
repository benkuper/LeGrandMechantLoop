/*
  ==============================================================================

	NodeConnectionManagerViewUI.cpp
	Created: 16 Nov 2020 10:00:57am
	Author:  bkupe

  ==============================================================================
*/

#include "NodeConnectionManagerViewUI.h"
#include "../../ui/NodeManagerViewUI.h"

NodeConnectionManagerViewUI::NodeConnectionManagerViewUI(NodeManagerViewUI* nodeManagerUI, NodeConnectionManager* manager) :
	BaseManagerUI(manager->niceName, manager),
	nodeManagerUI(nodeManagerUI)
{
	transparentBG = true;
	animateItemOnAdd = false;
	setInterceptsMouseClicks(false, true);
	removeChildComponent(addItemBT.get());

	addExistingItems();
}

NodeConnectionManagerViewUI::~NodeConnectionManagerViewUI()
{
}

NodeConnectionViewUI* NodeConnectionManagerViewUI::createUIForItem(NodeConnection* item)
{
	NodeViewUI* sourceUI = nodeManagerUI->getUIForItem(item->sourceNode);
	NodeViewUI* destUI = nodeManagerUI->getUIForItem(item->destNode);
	return new NodeConnectionViewUI(item, sourceUI, destUI);
}

void NodeConnectionManagerViewUI::addItemUIInternal(NodeConnectionViewUI* ui)
{
	ui->updateBounds();
}

void NodeConnectionManagerViewUI::resized()
{
	for (auto& ui : itemsUI) ui->updateBounds();
}
