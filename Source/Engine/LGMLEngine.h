/*
  ==============================================================================

    LGMLEngine.h
    Created: 15 Nov 2020 8:44:36am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"

class LGMLEngine :
    public Engine
{
public:
    LGMLEngine();
    ~LGMLEngine();

    void clearInternal() override;

    var getJSONData() override;
    void loadJSONDataInternalEngine(var data, ProgressTask* loadingTask) override;
};