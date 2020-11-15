/*
  ==============================================================================

    Interface.h
    Created: 15 Nov 2020 8:44:13am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"

class Interface :
    public BaseItem
{
public:
    Interface(StringRef name = "Interface", var params = var());
    virtual ~Interface();
};