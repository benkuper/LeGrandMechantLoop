/*
  ==============================================================================

    LGMLEngine.cpp
    Created: 15 Nov 2020 8:44:36am
    Author:  bkupe

  ==============================================================================
*/

#include "LGMLEngine.h"

#include "AudioManager.h"
#include "VSTManager.h"
#include "LGMLSettings.h"

#include "Node/NodeIncludes.h"
#include "Transport/Transport.h"
#include "Common/CommonIncludes.h"
#include "Preset/PresetIncludes.h"
//#include "Macro/MacroManager.h"
#include "Mapping/MappingManager.h"
#include "Interface/InterfaceIncludes.h"

LGMLEngine::LGMLEngine() :
    Engine("LGML File",".lgml")
{
    mainEngine = this;

    DecibelsHelpers::init();

    MIDIManager::getInstance(); //force init

    addChildControllableContainer(RootNodeManager::getInstance(), false, 0);
    addChildControllableContainer(Transport::getInstance(), false, 1);
    addChildControllableContainer(RootPresetManager::getInstance(), false, 2);
    //addChildControllableContainer(MacroManager::getInstance());
    addChildControllableContainer(MappingManager::getInstance(), false, 3);
    addChildControllableContainer(InterfaceManager::getInstance(), false, 4);

    ProjectSettings::getInstance()->addChildControllableContainer(AudioManager::getInstance());
    GlobalSettings::getInstance()->addChildControllableContainer(LGMLSettings::getInstance());
    GlobalSettings::getInstance()->addChildControllableContainer(VSTManager::getInstance());

    cpuUsage = addFloatParameter("Audio CPU Usage", "Audio CPU Usage indicator. /!\\ This is only showing the audio processing, not the UI and other processes !", 0, 0, 100);
    cpuUsage->setControllableFeedbackOnly(true);

    startTimer(2, 200);
}

LGMLEngine::~LGMLEngine()
{
    isClearing = true; 
    RootPresetManager::deleteInstance();
    MappingManager::deleteInstance();
    InterfaceManager::deleteInstance();
    //MacroManager::deleteInstance();
    RootNodeManager::deleteInstance();
    Transport::deleteInstance();

    AudioManager::deleteInstance();
    NodeFactory::deleteInstance();

    VSTManager::deleteInstance();
    LGMLSettings::deleteInstance();
    MIDIManager::deleteInstance();
}

void LGMLEngine::clearInternal()
{
    AudioManager::getInstance()->stop();
    RootPresetManager::getInstance()->clear();
    MappingManager::getInstance()->clear();
    InterfaceManager::getInstance()->clear();
    //MacroManager::getInstance()->clear();
    RootNodeManager::getInstance()->clear();
    Transport::getInstance()->clear();
}

var LGMLEngine::getJSONData()
{
    var data = Engine::getJSONData();
    data.getDynamicObject()->setProperty(RootNodeManager::getInstance()->shortName, RootNodeManager::getInstance()->getJSONData());
    data.getDynamicObject()->setProperty(Transport::getInstance()->shortName, Transport::getInstance()->getJSONData());
    //data.getDynamicObject()->setProperty(MacroManager::getInstance()->shortName, MacroManager::getInstance()->getJSONData());
    data.getDynamicObject()->setProperty(MappingManager::getInstance()->shortName, MappingManager::getInstance()->getJSONData());
    data.getDynamicObject()->setProperty(RootPresetManager::getInstance()->shortName, RootPresetManager::getInstance()->getJSONData());
    data.getDynamicObject()->setProperty(InterfaceManager::getInstance()->shortName, InterfaceManager::getInstance()->getJSONData());
    return data;
}

void LGMLEngine::loadJSONDataInternalEngine(var data, ProgressTask* loadingTask)
{
    Transport::getInstance()->loadJSONData(data.getProperty(Transport::getInstance()->shortName, var()));
    //MacroManager::getInstance()->loadJSONData(data.getProperty(MacroManager::getInstance()->shortName, var()));
    InterfaceManager::getInstance()->loadJSONData(data.getProperty(InterfaceManager::getInstance()->shortName, var()));
    MappingManager::getInstance()->loadJSONData(data.getProperty(MappingManager::getInstance()->shortName, var()));
    RootNodeManager::getInstance()->loadJSONData(data.getProperty(RootNodeManager::getInstance()->shortName, var()));
    RootPresetManager::getInstance()->loadJSONData(data.getProperty(RootPresetManager::getInstance()->shortName, var()));
}

void LGMLEngine::timerCallback(int timerID)
{
    Engine::timerCallback(timerID);
    if (isClearing || isLoadingFile) return;
    if (timerID == 2) cpuUsage->setValue(AudioManager::getInstance()->am.getCpuUsage() * 100);
}

String LGMLEngine::getMinimumRequiredFileVersion()
{
    return "1.0.0b1";
}

