/*
  ==============================================================================

    NodeFactory.h
    Created: 15 Nov 2020 8:56:42am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

class NodeFactory :
    public Factory<Node>
{
public:
    juce_DeclareSingleton(NodeFactory, true);
    
    NodeFactory();
    ~NodeFactory() {}
};