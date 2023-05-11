#include "MainComponent.h"

#include "Engine/LGMLEngine.h"
#include "Node/NodeIncludes.h"
#include "Preset/PresetIncludes.h"
#include "Outliner/LGMLOutliner.h"
#include "Transport/ui/TransportUI.h"
#include "Macro/ui/MacroManagerUI.h"
#include "Mapping/ui/MappingManagerUI.h"
#include "Interface/InterfaceIncludes.h"

//==============================================================================
String getAppVersion();
ApplicationProperties& getAppProperties();
ApplicationCommandManager& getCommandManager();

MainComponent::MainComponent()
{
	setSize(800, 600);
	getCommandManager().registerAllCommandsForTarget(this);
}

MainComponent::~MainComponent()
{
}

void MainComponent::init()
{
	ControllableUI::customAddToContextMenuFunc = MainComponent::addControllableMenuItems;
	ControllableUI::handleCustomContextMenuResultFunc = MainComponent::handleControllableMenuResult;

	ShapeShifterFactory::getInstance()->defs.add(new ShapeShifterDefinition("Nodes (List View)", &NodeManagerPanel::create));
	ShapeShifterFactory::getInstance()->defs.add(new ShapeShifterDefinition("Nodes (2D View)", &NodeManagerViewPanel::create));
	ShapeShifterFactory::getInstance()->defs.add(new ShapeShifterDefinition("Transport", &TransportUI::create));
	ShapeShifterFactory::getInstance()->defs.add(new ShapeShifterDefinition("Presets", &RootPresetManagerUI::create));
	//ShapeShifterFactory::getInstance()->defs.add(new ShapeShifterDefinition("Macros", &MacroManagerUI::create));
	ShapeShifterFactory::getInstance()->defs.add(new ShapeShifterDefinition("Mappings", &MappingManagerUI::create));
	ShapeShifterFactory::getInstance()->defs.add(new ShapeShifterDefinition("Interfaces", &InterfaceManagerUI::create));

	ControllableUI::drawContourOnInspectableHighlighted = true;

	OrganicMainContentComponent::init();

	String lastVersion = getAppProperties().getUserSettings()->getValue("lastVersion", "");

	if (lastVersion != getAppVersion())
	{
		//WelcomeScreen w;
		//DialogWindow::showModalDialog("Welcome", &w, getTopLevelComponent(), Colours::black, true);
	}
}

Outliner* MainComponent::createOutliner(const String& contentName)
{
	return new LGMLOutliner(contentName);
}

void MainComponent::addControllableMenuItems(ControllableUI* ui, PopupMenu* p)
{
	if (ui->controllable->isControllableFeedbackOnly) return;

	Controllable* c = ui->controllable.get();
	bool isPresettable = RootPresetManager::getInstance()->isControllablePresettable(c);
	p->addItem(0x5000, "Presettable", true, isPresettable);

	if (isPresettable)
	{
		Preset* preset = RootPresetManager::getInstance()->currentPreset;
		String presetName = preset != nullptr ? preset->niceName : "No loaded preset";
		String add = ui->controllable->getControlAddress();
		bool canBeOverride = preset != nullptr && !preset->isMain();
		bool isOverride = canBeOverride && preset->overridenControllables.contains(add);
		bool isIgnored = preset != nullptr ? preset->ignoredControllables.contains(c) : false;

		p->addItem(0x5001, "Save to current (" + presetName + ")", true, isOverride);

		PopupMenu saveToOtherMenu;
		RootPresetManager::getInstance()->fillPresetMenu(saveToOtherMenu, 0x20000, c, true, [](Preset* p, Controllable* c) { return p->hasPresetControllable(c); });

		p->addSubMenu("Save to...", saveToOtherMenu);

		p->addItem(0x5002, "Remove from current (" + presetName + ")", isOverride);
		PopupMenu removeFromMenu;
		RootPresetManager::getInstance()->fillPresetMenu(removeFromMenu, 0x30000, c);
		p->addSubMenu("Remove from...", removeFromMenu);
		p->addItem(0x5003, "Remove from all");

		p->addItem(0x5004, "Ignore in current (" + presetName + ")", preset != nullptr, isIgnored);
		PopupMenu ignoreMenu;
		RootPresetManager::getInstance()->fillPresetMenu(ignoreMenu, 0x40000, c, false, [](Preset* p, Controllable* c) { return p->ignoredControllables.contains(c); });
		p->addSubMenu("Ignore in...", ignoreMenu);
	}

	p->addSeparator();

	PopupMenu mappingMenu;
	mappingMenu.addItem(0x6001, MIDIMapping::getTypeStringStatic());
	mappingMenu.addItem(0x6002, MacroMapping::getTypeStringStatic());
	mappingMenu.addItem(0x6003, GenericMapping::getTypeStringStatic());
	p->addSubMenu("Create Mapping...", mappingMenu);

	Mapping* cm = MappingManager::getInstance()->getMappingForDestControllable(c);
	p->addItem(0x6100, "Select Mapping", cm != nullptr);

}

bool MainComponent::handleControllableMenuResult(ControllableUI* ui, int result)
{
	if (result < 0x5000 && result > 0x5020) return false;

	Controllable* c = ui->controllable.get();

	if (result == 0x5000)
	{
		RootPresetManager::getInstance()->toggleControllablePresettable(c);
		return true;
	}
	else if (result == 0x5001 || result == 0x5002)
	{
		if (Preset* preset = RootPresetManager::getInstance()->currentPreset)
		{
			String add = c->getControlAddress();
			if (result == 0x5001)
			{
				preset->save(c);
				LOG("Saved " << c->niceName << " to preset " << preset->niceName << (preset->isMain() ? "" : " (Override)"));
			}
			else if (result == 0x5002)
			{
				preset->removeControllableFromDataMap(c);
				preset->load(c);
				LOG("Removed " << c->niceName << " from preset " << preset->niceName << (preset->isMain() ? "" : " (Override)"));
			}

		}
		return true;
	}
	else if (result == 0x5003)
	{
		AlertWindow::showOkCancelBox(AlertWindow::QuestionIcon, "Remove from all presets", "Are you sure you want to remove this parameter from all presets ?", "Yes", "No", nullptr, ModalCallbackFunction::create([c](int result)
			{
				if (result)
				{
					RootPresetManager::getInstance()->removeAllPresetsFor(c);
					LOG("Removed " << c->niceName << " from all presets");
				}

			})
		);
		return true;
	}
	else if (result == 0x5004 || (result >= 0x40000 && result < 0x50000))
	{
		Preset* preset = nullptr;
		if (result == 0x5004) preset = RootPresetManager::getInstance()->currentPreset;
		else preset = RootPresetManager::getInstance()->getPresetForMenuResult(result - 0x40000);

		if (preset != nullptr)
		{
			String add = c->getControlAddress();
			if (preset->ignoredControllables.contains(c))
			{
				preset->ignoredControllables.removeAllInstancesOf(c);
				LOG("Removed " << c->niceName << " from ignore list in preset " << preset->niceName);
			}
			else
			{
				preset->ignoredControllables.add(c);
				LOG("Added " << c->niceName << " to ignore list in preset " << preset->niceName);
			}

		}

		return true;
	}
	else if (result >= 0x20000 && result < 0x30000)
	{
		if (Preset* preset = RootPresetManager::getInstance()->getPresetForMenuResult(result - 0x20000)) preset->save(c);
		return true;
	}
	else if (result >= 0x30000 && result < 0x40000)
	{
		if (Preset* preset = RootPresetManager::getInstance()->getPresetForMenuResult(result - 0x30000)) preset->removeControllableFromDataMap(c);
		return true;
	}

	else if (result >= 0x6001 && result <= 0x6003)
	{
		if (result >= 0x6001) MappingManager::getInstance()->createMappingForControllable(c, MIDIMapping::getTypeStringStatic());
		if (result >= 0x6002) MappingManager::getInstance()->createMappingForControllable(c, MacroMapping::getTypeStringStatic());
		if (result >= 0x6003) MappingManager::getInstance()->createMappingForControllable(c, GenericMapping::getTypeStringStatic());

		return true;
	}
	else if (result == 0x6100)
	{
		Mapping* cm = MappingManager::getInstance()->getMappingForDestControllable(c);
		if (cm != nullptr) cm->selectThis();
		return true;
	}

	return false;
}

LGMLMenuBarComponent::LGMLMenuBarComponent(MainComponent* mainComp, LGMLEngine* engine) :
	Component("LGML Menu Bar")
#if !JUCE_MAC
	, menuBarComp(mainComp)
#endif
	, audioSettingsUI("audioSettings")
{
#if !JUCE_MAC
	addAndMakeVisible(menuBarComp);
#endif

	cpuUsageUI.reset(engine->cpuUsage->createSlider());
	cpuUsageUI->suffix = " %";
	cpuUsageUI->fixedDecimals = -1; //only int

	addAndMakeVisible(cpuUsageUI.get());

	logInUI.reset(OSCRemoteControl::getInstance()->logIncoming->createToggle());
	logOutUI.reset(OSCRemoteControl::getInstance()->logOutgoing->createToggle());

	addAndMakeVisible(logInUI.get());
	addAndMakeVisible(logOutUI.get());

	audioSettingsUI.setFont(10);
	audioSettingsUI.setJustificationType(Justification::centredRight);
	audioSettingsUI.setText(AudioManager::getInstance()->getCurrentDeviceDescription(), dontSendNotification);
	addAndMakeVisible(audioSettingsUI);

	AudioManager::getInstance()->addAudioManagerListener(this);
}

LGMLMenuBarComponent::~LGMLMenuBarComponent()
{
	if(AudioManager::getInstanceWithoutCreating() != nullptr)
		AudioManager::getInstance()->removeAudioManagerListener(this);
}

void LGMLMenuBarComponent::paint(Graphics& g)
{
	g.fillAll(BG_COLOR);
}

void LGMLMenuBarComponent::resized()
{
	Rectangle<int> r = getLocalBounds();
#if !JUCE_MAC	
	menuBarComp.setBounds(r);
#endif
	logOutUI->setBounds(r.removeFromRight(90).reduced(1)); //overlap but we don't care
	r.removeFromRight(20);
	logInUI->setBounds(r.removeFromRight(90).reduced(1)); //overlap but we don't care
	r.removeFromRight(20);
	cpuUsageUI->setBounds(r.removeFromRight(200).reduced(2)); //overlap but we don't care
	r.removeFromRight(8);
	audioSettingsUI.setBounds(r.removeFromRight(400));

}

void LGMLMenuBarComponent::audioSetupChanged()
{

	audioSettingsUI.setColour(Label::ColourIds::textColourId, AudioManager::getInstance()->am.getCurrentAudioDevice() != nullptr ? TEXT_COLOR : RED_COLOR);
	audioSettingsUI.setText(AudioManager::getInstance()->getCurrentDeviceDescription(), dontSendNotification);
}
