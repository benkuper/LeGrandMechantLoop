/*
  ==============================================================================

    NodeConnectionEditor.h
    Created: 16 Nov 2020 10:01:11am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "../NodeConnection.h"

class NodeConnectionEditor :
    public BaseItemEditor
{
public:
    NodeConnectionEditor(NodeConnection* connection, bool isRoot);
    ~NodeConnectionEditor();

    NodeConnection* connection;
};