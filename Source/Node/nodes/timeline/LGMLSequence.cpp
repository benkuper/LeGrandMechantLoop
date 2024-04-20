/*
  ==============================================================================

	LGMLSequence.cpp
	Created: 20 Apr 2024 9:05:28pm
	Author:  bkupe

  ==============================================================================
*/

#include "Node/NodeIncludes.h"
#include "LGMLSequence.h"

LGMLSequence::LGMLSequence(TimelineNode* node) :
	node(node)
{
	setAudioDeviceManager(&AudioManager::getInstance()->am);

	layerManager->factory.defs.add(SequenceLayerManager::LayerDefinition::createDef("", "Audio", &LGMLAudioLayer::create, this, true));
	layerManager->addBaseManagerListener(this);
}

LGMLSequence::~LGMLSequence()
{

}

void LGMLSequence::clearItem()
{
	Sequence::clearItem();
	layerManager->removeBaseManagerListener(this);
	for (auto& layer : layerManager->items)
	{
		if (LGMLAudioLayer* audioLayer = dynamic_cast<LGMLAudioLayer*>(layer))
		{
			audioLayer->setAudioProcessorGraph(nullptr);
		}
	}
}

void LGMLSequence::itemAdded(SequenceLayer* layer)
{
	if (LGMLAudioLayer* audioLayer = dynamic_cast<LGMLAudioLayer*>(layer))
	{
		audioLayer->setAudioProcessorGraph(&node->containerGraph, node->audioOutputID);
	}
}

void LGMLSequence::itemsAdded(Array<SequenceLayer*> layers)
{
	for (auto& layer : layers)
	{
		if (LGMLAudioLayer* audioLayer = dynamic_cast<LGMLAudioLayer*>(layer))
		{
			audioLayer->setAudioProcessorGraph(&node->containerGraph, node->audioOutputID);
		}
	}
}

void LGMLSequence::updateAudioInputs()
{
	for (auto& l : layerManager->items)
	{
		if (LGMLAudioLayer* audioLayer = dynamic_cast<LGMLAudioLayer*>(l))
		{
			audioLayer->updateAudioInputs();
		}
	}
}

void LGMLSequence::updateAudioOutputs()
{
	for (auto& l : layerManager->items)
	{
		if (LGMLAudioLayer* audioLayer = dynamic_cast<LGMLAudioLayer*>(l))
		{
			audioLayer->updateAudioOutputs();
		}
	}
}

LGMLAudioLayer::LGMLAudioLayer(Sequence* s, var params) :
	AudioLayer(s, params)
{
}

LGMLAudioLayer::~LGMLAudioLayer()
{
}

void LGMLAudioLayer::updateAudioInputs()
{
}

void LGMLAudioLayer::updateAudioOutputs()
{
	LGMLSequence* seq = dynamic_cast<LGMLSequence*>(sequence);
	setAudioProcessorGraph(&seq->node->containerGraph, seq->node->audioOutputID);
}

int LGMLAudioLayer::getNodeGraphIDIncrement()
{
	return AudioManager::getInstance()->getNewGraphID();
}
