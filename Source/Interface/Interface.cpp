/*
  ==============================================================================

    Interface.cpp
    Created: 15 Nov 2020 8:44:13am
    Author:  bkupe

  ==============================================================================
*/

#include "LGMLAssetManager.h"

Interface::Interface(StringRef name, var params) :
    BaseItem(name, true, true)
{
    scriptManager->scriptTemplate = LGMLAssetManager::getScriptTemplateBundle(StringArray("generic", "interface"));

    logIncomingData = addBoolParameter("Log Incoming Data", "", false);
    logOutgoingData = addBoolParameter("Log Outgoing Data", "", false);
}

Interface::~Interface()
{
}
