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

static OrganicApplication& getApp() { return *dynamic_cast<OrganicApplication*>(JUCEApplication::getInstance()); }

VSTManager::VSTManager() :
    ControllableContainer("VST Plugin Paths"),
    Thread("VST Scan"),
    vstManagerNotifier(5),
    scanAU(nullptr)
{
    rescan = addTrigger("Rescan", "Rescan all paths");
    rescan->hideInEditor = true;

    scanVST = addBoolParameter("Scan VST", "Scan VST Plugins", true);
#if JUCE_MAC
    scanAU = addBoolParameter("Scan AU", "Scan AU Plugins", true);
#endif

    updateVSTFormats();

    userCanAddControllables = true;
    userAddControllablesFilters.add(FileParameter::getTypeStringStatic());
}

VSTManager::~VSTManager()
{
    idDescriptionMap.clear();
    descriptions.clear();
}

void VSTManager::updateVSTFormats()
{
    formatManager.reset(new AudioPluginFormatManager());

#if JUCE_PLUGINHOST_AU && (JUCE_MAC || JUCE_IOS)
    if (scanAU->boolValue()) formatManager->addFormat(new AudioUnitPluginFormat());
#endif

#if JUCE_PLUGINHOST_VST && (JUCE_MAC || JUCE_WINDOWS || JUCE_LINUX || JUCE_IOS)
    if (scanVST->boolValue()) formatManager->addFormat(new VSTPluginFormat());
#endif

#if JUCE_PLUGINHOST_VST3 && (JUCE_MAC || JUCE_WINDOWS || JUCE_LINUX)
    if (scanVST->boolValue()) formatManager->addFormat(new VST3PluginFormat());
#endif

#if JUCE_PLUGINHOST_LADSPA && JUCE_LINUX
    formatManager->addFormat(new LADSPAPluginFormat());
#endif
}

void VSTManager::updateVSTList()
{
    LOG("Updating VSTs...");
   
    Array<AudioPluginFormat*> formats = formatManager->getFormats();
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
            LOG("Scanning " + fp + "...");
            f->findAllTypesForFile(descriptions, fp);
        }
    }

    String s = "Found plugins :";
    for (auto& d : descriptions)
    {
        idDescriptionMap.set(d->fileOrIdentifier, d);
        s += "\n" + d->category + " > " + d->name + " (" + d->pluginFormatName + ")";
    }
    
    NLOG("VST", s);

    getApp().saveGlobalSettings();

    vstManagerNotifier.addMessage(new VSTManagerEvent(VSTManagerEvent::PLUGINS_UPDATED, this));
}

void VSTManager::onContainerParameterChanged(Parameter* p)
{
    if (isCurrentlyLoadingData) return;

    if (p == scanVST || p == scanAU)
    {
        updateVSTFormats();
    }

    startThread();
}

void VSTManager::onControllableAdded(Controllable* c)
{
    if (FileParameter* fp = dynamic_cast<FileParameter*>(c))
    {
        if(!isCurrentlyLoadingData) fp->setNiceName(getUniqueNameInContainer("Path"));
        fp->directoryMode = true;
        fp->forceAbsolutePath = true;
    }
}

void VSTManager::onControllableRemoved(Controllable* c)
{
    if (isCurrentlyLoadingData) return;

    startThread();
}

void VSTManager::reset()
{
    for (auto& c : controllables)
    {
        if (FileParameter* fp = dynamic_cast<FileParameter*>(c)) fp->resetValue();
    }

    descriptions.clear();
    idDescriptionMap.clear();
}

var VSTManager::getJSONData()
{
    var data = ControllableContainer::getJSONData();
    var descData;
    for (auto& d : descriptions) descData.append(d->createXml()->toString());
    data.getDynamicObject()->setProperty("descriptions", descData);
    return data;
}

void VSTManager::loadJSONDataInternal(var data)
{
    var descData = data.getProperty("descriptions", var());
    for (int i = 0; i < descData.size();i++)
    {
        PluginDescription* d = new PluginDescription();
        std::unique_ptr<XmlElement> xml = parseXML(descData[i].toString());

        if (d->loadFromXml(*xml))
        {
            idDescriptionMap.set(d->fileOrIdentifier, d);
            descriptions.add(d);
        }
        else  LOGWARNING("Could not load VST from cache.");
    }
}

void VSTManager::afterLoadJSONDataInternal()
{
    if (descriptions.isEmpty())
    {
        LOG("No VST loaded from cache, try to scan");
        startThread(); //not async to ensure file is loaded after first listing. Can be improved by handling async load in VST Nodes and connections
    }
}

void VSTManager::onContainerTriggerTriggered(Trigger* t)
{
    if (t == rescan) startThread();
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
