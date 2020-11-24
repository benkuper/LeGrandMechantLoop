/*
  ==============================================================================

    VSTManager.cpp
    Created: 24 Nov 2020 9:39:06am
    Author:  bkupe

  ==============================================================================
*/

#include "VSTManager.h"
#include "ui/VSTManagerUI.h"

juce_ImplementSingleton(VSTManager)

VSTManager::VSTManager() :
    ControllableContainer("VST Plugin Paths"),
    Thread("VST Scan"),
    vstManagerNotifier(5)
{
    rescan = addTrigger("Rescan", "Rescan all paths");
    rescan->hideInEditor = true;

    formatManager.addDefaultFormats();

    userCanAddControllables = true;
    userAddControllablesFilters.add(FileParameter::getTypeStringStatic());
}

VSTManager::~VSTManager()
{
    idDescriptionMap.clear();
    descriptions.clear();
}

void VSTManager::updateVSTList()
{
    Array<AudioPluginFormat*> formats = formatManager.getFormats();
    FileSearchPath searchPath;

    for (auto& c : controllables)
    {
        if (FileParameter* fp = dynamic_cast<FileParameter*>(c))
        {
            File f = ((FileParameter*)c)->getFile();
            if (f.isDirectory()) searchPath.add(f);
        }
    }

    idDescriptionMap.clear();
    descriptions.clear();
    for (auto& f : formats)
    {
        StringArray foundPlugins = f->searchPathsForPlugins(searchPath, true);
        for (auto& fp : foundPlugins)
        {
            f->findAllTypesForFile(descriptions, fp);
        }
    }

    for (auto& d : descriptions)
    {
        idDescriptionMap.set(d->fileOrIdentifier, d);
    }

    String s = "Found plugins :";
    for (auto& d : descriptions)
    {
        s += "\n" + d->category + " > " + d->name + " (" + d->pluginFormatName + ")";
    }
    
    NLOG("VST", s);

    vstManagerNotifier.addMessage(new VSTManagerEvent(VSTManagerEvent::PLUGINS_UPDATED, this));
}

void VSTManager::onContainerParameterChanged(Parameter* p)
{
    if (isCurrentlyLoadingData) return;

    startThread();
}

void VSTManager::onControllableAdded(Controllable* c)
{
    if (FileParameter* fp = dynamic_cast<FileParameter*>(c))
    {
        if(!isCurrentlyLoadingData) fp->setNiceName(getUniqueNameInContainer("Path"));
        fp->directoryMode = true;
    }
}

void VSTManager::onControllableRemoved(Controllable* c)
{
    if (isCurrentlyLoadingData) return;

    startThread();
}

var VSTManager::getJSONData()
{
    var data = ControllableContainer::getJSONData();
    return data;
}

void VSTManager::loadJSONDataInternal(var data)
{
}

void VSTManager::afterLoadJSONDataInternal()
{
    startThread();
}

void VSTManager::onContainerTriggerTriggered(Trigger* t)
{
    if (t == rescan) updateVSTList();
}

void VSTManager::run()
{
    updateVSTList();
}

InspectableEditor* VSTManager::getEditor(bool isRoot)
{
    return new VSTManagerEditor(this, isRoot);
}

VSTPluginParameter::VSTPluginParameter(const String& name, const String& description) :
    StringParameter(name, description, "")
{
}

VSTPluginParameter::~VSTPluginParameter()
{
}

PluginDescription* VSTPluginParameter::getPluginDescription()
{
    if (stringValue().isEmpty() || !VSTManager::getInstance()->idDescriptionMap.contains(stringValue())) return nullptr;
    return VSTManager::getInstance()->idDescriptionMap[stringValue()];
}

VSTPluginParameterUI* VSTPluginParameter::createVSTParamUI()
{
    return new VSTPluginParameterUI(this);
}

ControllableUI* VSTPluginParameter::createDefaultUI()
{
    return createVSTParamUI();
}
