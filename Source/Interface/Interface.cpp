/*
  ==============================================================================

    Interface.cpp
    Created: 15 Nov 2020 8:44:13am
    Author:  bkupe

  ==============================================================================
*/

#include "Interface/InterfaceIncludes.h"
#include "LGMLAssetManager.h"

Interface::Interface(StringRef name, var params) :
    BaseItem(name, true, true)
{
    scriptManager->scriptTemplate = LGMLAssetManager::getScriptTemplateBundle(StringArray("generic", "interface"));

    logIncomingData = addBoolParameter("Log Incoming Data", "", false);
    logOutgoingData = addBoolParameter("Log Outgoing Data", "", false);

    inActivityTrigger.reset(new Trigger("IN Activity", "Incoming Activity Signal"));
    outActivityTrigger.reset(new Trigger("OUT Activity", "Outgoing Activity Signal"));
}

Interface::~Interface()
{
}
