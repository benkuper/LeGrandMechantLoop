/*
  ==============================================================================

    LGMLAssetManager.h
    Created: 26 May 2021 1:31:25pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"

class LGMLAssetManager
{
public:
    static String getScriptTemplate(const String& templateRef)
    {
        int templateDataSize = 0;
        String resourceName = templateRef + "ScriptTemplate_js";
        const char* result = BinaryData::getNamedResource(resourceName.getCharPointer(), templateDataSize);
        if (templateDataSize == 0)
        {
            DBG("Template not found : " << templateRef);
            return "";
        }

        return String("\n" + String(result) + "\n");
    }

    static String getScriptTemplateBundle(StringArray templateRefs)
    {
        String result = "";
        for (auto& s : templateRefs)
        {
            result += getScriptTemplate(s);
        }

        return result;
    }

};
