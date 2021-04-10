/*
  ==============================================================================

    LGMLEngine.cpp
    Created: 15 Nov 2020 8:44:36am
    Author:  bkupe

  ==============================================================================
*/

#include "LGMLEngine.h"

#include "Node/NodeManager.h"
#include "Node/NodeFactory.h"
#include "Interface/InterfaceManager.h"
#include "Transport/Transport.h"
#include "AudioManager.h"
#include "VSTManager.h"
#include "LGMLSettings.h"
#include "Common/MIDI/MIDIManager.h"

LGMLEngine::LGMLEngine() :
    Engine("LGML File",".lgml")
{
    mainEngine = this;

    addChildControllableContainer(RootNodeManager::getInstance());
    addChildControllableContainer(InterfaceManager::getInstance());
    addChildControllableContainer(Transport::getInstance());

    ProjectSettings::getInstance()->addChildControllableContainer(AudioManager::getInstance());
    GlobalSettings::getInstance()->addChildControllableContainer(LGMLSettings::getInstance());
    GlobalSettings::getInstance()->addChildControllableContainer(VSTManager::getInstance());

    cpuUsage = addFloatParameter("Audio CPU Usage", "Audio CPU Usage indicator. /!\\ This is only showing the audio processing, not the UI and other processes !", 0, 0, 1);
    cpuUsage->setControllableFeedbackOnly(true);

    startTimer(2, 500);
}

LGMLEngine::~LGMLEngine()
{
    isClearing = true; 
    RootNodeManager::deleteInstance();
    InterfaceManager::deleteInstance();
    Transport::deleteInstance();

    AudioManager::deleteInstance();
    NodeFactory::deleteInstance();

    VSTManager::deleteInstance();
    LGMLSettings::deleteInstance();
    MIDIManager::deleteInstance();
}

void LGMLEngine::clearInternal()
{
    RootNodeManager::getInstance()->clear();
    InterfaceManager::getInstance()->clear();
    Transport::getInstance()->clear();
}

var LGMLEngine::getJSONData()
{
    var data = Engine::getJSONData();
    data.getDynamicObject()->setProperty(RootNodeManager::getInstance()->shortName, RootNodeManager::getInstance()->getJSONData());
    data.getDynamicObject()->setProperty(InterfaceManager::getInstance()->shortName, InterfaceManager::getInstance()->getJSONData());
    data.getDynamicObject()->setProperty(Transport::getInstance()->shortName, Transport::getInstance()->getJSONData());
    return data;
}

void LGMLEngine::loadJSONDataInternalEngine(var data, ProgressTask* loadingTask)
{
    RootNodeManager::getInstance()->loadJSONData(data.getProperty(RootNodeManager::getInstance()->shortName, var()));
    InterfaceManager::getInstance()->loadJSONData(data.getProperty(InterfaceManager::getInstance()->shortName, var()));
    Transport::getInstance()->loadJSONData(data.getProperty(Transport::getInstance()->shortName, var()));
}

void LGMLEngine::timerCallback(int timerID)
{
    Engine::timerCallback(timerID);
    if (isClearing || isLoadingFile) return;
    if (timerID == 2) cpuUsage->setValue(AudioManager::getInstance()->am.getCpuUsage());
}

String LGMLEngine::getMinimumRequiredFileVersion()
{
    return "1.0.0b1";
}

