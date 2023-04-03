/*
  ==============================================================================

	VSTRackNode.cpp
	Created: 29 Apr 2021 2:03:59pm
	Author:  bkupe

  ==============================================================================
*/

#include "VSTRackNode.h"

VSTRackNode::VSTRackNode() :
	Node("VST Rack"),
	manager("items")
{

}

VSTRackNode::~VSTRackNode()
{
}

void VSTRackNode::addVSTFromDescription(PluginDescription* d)
{
}

VSTRackItem::VSTRackItem(var params)
{
}

VSTRackItem::~VSTRackItem()
{
}
