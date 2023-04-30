/*
  ==============================================================================

	NodeConnectionManagerViewUI.cpp
	Created: 16 Nov 2020 10:00:57am
	Author:  bkupe

  ==============================================================================
*/

#include "Node/NodeIncludes.h"

NodeConnectionManagerViewUI::NodeConnectionManagerViewUI(NodeManagerViewUI* nodeManagerUI, NodeConnectionManager* manager) :
	BaseManagerUI(manager->niceName, manager, false),
	nodeManagerUI(nodeManagerUI)
{
	bringToFrontOnSelect = false;
	autoFilterHitTestOnItems = true;
	validateHitTestOnNoItem = false;

	setRepaintsOnMouseActivity(true);

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
	BaseNodeViewUI* sourceUI = nodeManagerUI->getUIForItem(item->sourceNode);
	BaseNodeViewUI* destUI = nodeManagerUI->getUIForItem(item->destNode);

	NodeConnector* sourceConnector = nullptr;
	if(sourceUI != nullptr) sourceConnector = item->connectionType == NodeConnection::AUDIO ? sourceUI->outAudioConnector.get() : sourceUI->outMIDIConnector.get();
	NodeConnector* destConnector = nullptr;
	if(destUI != nullptr) destConnector = item->connectionType == NodeConnection::AUDIO ? destUI->inAudioConnector.get() : destUI->inMIDIConnector.get();

	return new NodeConnectionViewUI(item, sourceConnector, destConnector);
}

void NodeConnectionManagerViewUI::placeItems(Rectangle<int>& r)
{
	for (auto& ui : itemsUI) ui->updateBounds();
}

void NodeConnectionManagerViewUI::addItemUIInternal(NodeConnectionViewUI* ui)
{
	ui->updateBounds();
}

void NodeConnectionManagerViewUI::resized()
{
}