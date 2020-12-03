/*
  ==============================================================================

    NodeFactory.h
    Created: 15 Nov 2020 8:56:42am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "Node.h"

/*
class NodeManager;

typedef std::function<Node* (NodeManager*, var)> NodeCreateFunc;

class FactoryNodeDefinition :
    public FactoryParametricDefinition <Node, NodeCreateFunc>
{
public:
    FactoryNodeDefinition(StringRef menuPath, StringRef type, NodeCreateFunc createFunc, NodeManager* nodeManager, var params = new DynamicObject()) :
        FactoryParametricDefinition(menuPath, type, createFunc, params),
        nodeManager(nodeManager)
    {}

    NodeManager* nodeManager;
};
*/

class NodeFactory :
    public Factory<Node>
{
public:
    juce_DeclareSingleton(NodeFactory, true);
    
    NodeFactory();
    ~NodeFactory() {}
};