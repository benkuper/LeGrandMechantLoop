/*
  ==============================================================================

    NodeConnectionEditor.cpp
    Created: 16 Nov 2020 10:01:11am
    Author:  bkupe

  ==============================================================================
*/

#include "NodeConnectionEditor.h"

NodeConnectionEditor::NodeConnectionEditor(NodeConnection* connection, bool isRoot) :
    BaseItemEditor(connection, isRoot),
    connection(connection)
{

}

NodeConnectionEditor::~NodeConnectionEditor()
{

}
