/*
  ==============================================================================

	NodeConnectionManagerViewUI.cpp
	Created: 16 Nov 2020 10:00:57am
	Author:  bkupe

  ==============================================================================
*/

#include "NodeConnectionManagerViewUI.h"
#include "../../ui/NodeManagerViewUI.h"
#include "NodeConnector.h"

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
	NodeViewUI* sourceUI = nodeManagerUI->getUIForItem(item->sourceNode);
	NodeViewUI* destUI = nodeManagerUI->getUIForItem(item->destNode);

	return new NodeConnectionViewUI(item, sourceUI->outAudioConnector.get(), destUI->inAudioConnector.get());
}

void NodeConnectionManagerViewUI::placeItems(Rectangle<int> &r)
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

void NodeConnectionManagerViewUI::startCreateConnection(NodeConnector* connector, NodeConnection* connectionToReplace)
{
	if (connector == nullptr) return;

	tmpConnectionToReplace = connectionToReplace;
	if (NodeConnectionViewUI * cui = getUIForItem(tmpConnectionToReplace)) cui->setAlpha(.5f);
	tmpConnectionLookForInput = !connector->isInput;
	tmpConnectionUI.reset(new NodeConnectionViewUI(nullptr, connector->isInput  ? nullptr : connector, connector->isInput ? connector : nullptr));
	addAndMakeVisible(tmpConnectionUI.get());
}

void NodeConnectionManagerViewUI::updateCreateConnection()
{
	if (tmpConnectionUI == nullptr) return;

	NodeViewUI* excludeUI = (tmpConnectionLookForInput  ? tmpConnectionUI->sourceConnector : tmpConnectionUI->destConnector)->nodeViewUI;
	NodeConnector* candidateConnector = nodeManagerUI->getCandidateConnector(tmpConnectionLookForInput, excludeUI);
	

	if (tmpConnectionLookForInput) tmpConnectionUI->setDestConnector(candidateConnector);
	else tmpConnectionUI->setSourceConnector(candidateConnector);
	tmpConnectionUI->updateBounds();
}


void NodeConnectionManagerViewUI::endCreateConnection()
{
	if (tmpConnectionUI == nullptr) return;
	
	if (tmpConnectionUI->sourceConnector != nullptr && tmpConnectionUI->destConnector != nullptr) 
	{
		if (tmpConnectionToReplace != nullptr
			&& tmpConnectionUI->sourceConnector->nodeViewUI->item == tmpConnectionToReplace->sourceNode
			&& tmpConnectionUI->destConnector->nodeViewUI->item == tmpConnectionToReplace->destNode)
		{
			//same, do nothing
		}
		else
		{
			Array<UndoableAction*> actions;
			var channelMapData;
			if (tmpConnectionToReplace != nullptr)
			{
				channelMapData = tmpConnectionToReplace->getChannelMapData();
				actions.addArray(manager->getRemoveItemUndoableAction(tmpConnectionToReplace));
			}

			actions.addArray(manager->getAddConnectionUndoableAction(tmpConnectionUI->sourceConnector->nodeViewUI->item, tmpConnectionUI->destConnector->nodeViewUI->item, channelMapData));

			UndoMaster::getInstance()->performActions("Move Connection", actions);
		}
	}

	if (NodeConnectionViewUI* cui = getUIForItem(tmpConnectionToReplace)) cui->setAlpha(1);

	tmpConnectionToReplace = nullptr;
	removeChildComponent(tmpConnectionUI.get());
	tmpConnectionUI.reset();
}