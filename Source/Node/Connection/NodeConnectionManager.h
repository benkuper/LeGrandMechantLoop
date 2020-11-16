/*
  ==============================================================================

    NodeConnectionManager.h
    Created: 16 Nov 2020 10:00:12am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "NodeConnection.h"

class NodeConnectionManager :
    public BaseManager<NodeConnection>
{
public:
    NodeConnectionManager();
    ~NodeConnectionManager();

};