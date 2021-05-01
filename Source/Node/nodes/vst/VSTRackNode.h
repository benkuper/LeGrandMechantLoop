/*
  ==============================================================================

    VSTRackNode.h
    Created: 29 Apr 2021 2:03:59pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

class VSTRackItem :
    public BaseItem
{
public:
    VSTRackItem(var params = var());
    ~VSTRackItem();
};


class VSTRackNode :
    public Node
{
public:
    VSTRackNode();
    ~VSTRackNode();

    BoolParameter* exclusive;
    BaseManager<VSTRackItem> manager;

    void addVSTFromDescription(PluginDescription* d);
};