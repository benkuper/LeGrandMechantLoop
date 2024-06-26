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
	vstManagerNotifier(5),
	scanAU(nullptr),
	scanLV2(nullptr)
{
	rescan = addTrigger("Rescan", "Rescan all paths");
	rescan->hideInEditor = true;

	scanVST = addBoolParameter("Scan VST", "Scan VST Plugins", true);
	scanVST->hideInEditor = true;
#if JUCE_MAC
	scanAU = addBoolParameter("Scan AU", "Scan AU Plugins", true);
	scanAU->hideInEditor = true;
#elif JUCE_LINUX
	scanLV2 = addBoolParameter("Scan LV2", "Scan LV2 Plugins", true);
	scanLV2->hideInEditor = true;
#endif

	userCanAddControllables = true;
	userAddControllablesFilters.add(FileParameter::getTypeStringStatic());

	updateVSTFormats();
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

#if JUCE_PLUGINHOST_LV2 && JUCE_LINUX
	if (scanLV2->boolValue()) formatManager->addFormat(new LV2PluginFormat());
#endif

}

void VSTManager::updateVSTList()
{
	LOG("Updating VSTs...");

	updateVSTFormats();

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
			try
			{
				LOG("Scanning " + fp + "...");
				f->findAllTypesForFile(descriptions, fp);
			}
			catch (std::exception e)
			{
				NLOGERROR("VST", "Error while scanning " << fp << " :\n " << e.what());
			}
		}
	}

	DescriptionSorter sorter;
	descriptions.sort(sorter, true);

	String s = "Found plugins :";
	for (auto& d : descriptions)
	{
		String pid = d->manufacturerName + "/" + d->name;
		idDescriptionMap.set(pid, d);
		s += "\n" + pid + ", uid : " + String(d->uniqueId) + " (" + d->pluginFormatName + ")";
	}

	NLOG("VST", s);

	getApp().saveGlobalSettings();

	vstManagerNotifier.addMessage(new VSTManagerEvent(VSTManagerEvent::PLUGINS_UPDATED, this));
}

void VSTManager::onContainerParameterChanged(Parameter* p)
{
	if (isCurrentlyLoadingData) return;
	//startThread();
}

void VSTManager::onControllableAdded(Controllable* c)
{
	if (FileParameter* fp = dynamic_cast<FileParameter*>(c))
	{
		if (!isCurrentlyLoadingData) fp->setNiceName(getUniqueNameInContainer("Path"));
		fp->directoryMode = true;
		fp->forceAbsolutePath = true;
	}
}

void VSTManager::onControllableRemoved(Controllable* c)
{
	if (isCurrentlyLoadingData) return;

	//startThread();
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
	for (int i = 0; i < descData.size(); i++)
	{
		PluginDescription* d = new PluginDescription();
		std::unique_ptr<XmlElement> xml = parseXML(descData[i].toString());

		if (d->loadFromXml(*xml))
		{
			String pid = d->manufacturerName + "/" + d->name;
			idDescriptionMap.set(pid, d);
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
	if (t == rescan)
	{
		updateVSTList();
	}
}

String VSTManager::getParameterValueForDescription(PluginDescription* d)
{
	if (d == nullptr) return "";
	return d->manufacturerName + "/" + d->name;
}

void VSTManager::run()
{
	updateVSTList();
}

InspectableEditor* VSTManager::getEditorInternal(bool isRoot, Array<Inspectable*> inspectables)
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
	if (value.toString().isNotEmpty() && VSTManager::getInstance()->idDescriptionMap.contains(value.toString())) return VSTManager::getInstance()->idDescriptionMap[value.toString()];

	if (vstID != -1 && VSTManager::getInstance()->uidDescriptionMap.contains(vstID)) return VSTManager::getInstance()->uidDescriptionMap[vstID];

	return nullptr;
}

void VSTPluginParameter::setValueInternal(var& value)
{
	StringParameter::setValueInternal(value);

	if (PluginDescription* d = getPluginDescription())
	{
		if (d != nullptr) vstID = d->uniqueId;

		String pid = VSTManager::getInstance()->getParameterValueForDescription(d);
		if (value.toString() != pid)
		{
			setValue(pid);
		}
	}
}

VSTPluginParameterUI* VSTPluginParameter::createVSTParamUI()
{
	return new VSTPluginParameterUI(this);
}

ControllableUI* VSTPluginParameter::createDefaultUI(Array<Controllable*> controllables)
{
	return createVSTParamUI();
}

var VSTPluginParameter::getJSONDataInternal()
{
	var data = StringParameter::getJSONDataInternal();
	data.getDynamicObject()->setProperty("vstID", vstID);
	return data;
}

void VSTPluginParameter::loadJSONDataInternal(var data)
{
	vstID = data.getProperty("vstID", -1);
	StringParameter::loadJSONDataInternal(data);
}

inline int DescriptionSorter::compareElements(PluginDescription* first, PluginDescription* second)
{
	int result = first->manufacturerName.compare(second->manufacturerName);
	if (result == 0) result = first->name.compare(second->name);
	return result;
}
