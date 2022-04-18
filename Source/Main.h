/*
  ==============================================================================

    Main.h
    Created: 15 Nov 2020 8:39:44am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class LGMLApplication : public OrganicApplication
{
public:
    //==============================================================================
    LGMLApplication();

    void initialiseInternal(const String& commandLine) override;
    void afterInit() override;

    void clearGlobalSettings() override;
}; 

START_JUCE_APPLICATION(LGMLApplication)