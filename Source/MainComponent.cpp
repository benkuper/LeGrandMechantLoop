#include "MainComponent.h"

#include "Engine/LGMLEngine.h"

#include  "Node/ui/NodeManagerUI.h"
#include  "Node/ui/NodeManagerViewUI.h"
#include "Interface/ui/InterfaceManagerUI.h"
#include "Transport/ui/TransportUI.h"
#include "Preset/ui/PresetManagerUI.h"
#include "Outliner/LGMLOutliner.h"

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
	//ShapeShifterFactory::getInstance()->defs.add(new ShapeShifterDefinition("Interfaces", &InterfaceManagerUI::create));
	ShapeShifterFactory::getInstance()->defs.add(new ShapeShifterDefinition("Transport", &TransportUI::create));
	ShapeShifterFactory::getInstance()->defs.add(new ShapeShifterDefinition("Presets", &RootPresetManagerUI::create));
	//ShapeShifterFactory::getInstance()->defs.add(new ShapeShifterDefinition("Timeline", &TimelineU::create));


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
	if (ui->controllable->type == Controllable::TRIGGER) return;

	bool isPresettable = ui->controllable->customData == "presettable";
	p->addItem(0x5000, "Presettable", true, isPresettable);
	
	if (isPresettable)
	{
		Preset* preset = RootPresetManager::getInstance()->currentPreset;
		String presetName = preset != nullptr ? preset->niceName : "No loaded preset";
		String add = ui->controllable->getControlAddress();
		bool canBeOverride = preset != nullptr && !preset->isMain();
		bool isOverride = canBeOverride && preset->overridenControllables.contains(add);
		p->addItem(0x5001, "Override (" + presetName + ")", canBeOverride, isOverride);
	}
}

bool MainComponent::handleControllableMenuResult(ControllableUI* ui, int result)
{
	if (ui->controllable->type == Controllable::TRIGGER) return false;
	
	Parameter* p = (Parameter*)ui->controllable.get();
	
	if (result == 0x5000)
	{
		if (p->customData == "presettable") p->customData = var();
		else p->customData = "presettable";
		return true;
	}
	else if (result == 0x5001)
	{
		if (Preset* preset = RootPresetManager::getInstance()->currentPreset)
		{
			String add = p->getControlAddress();
			if (preset->overridenControllables.contains(add))
			{
				preset->overridenControllables.removeAllInstancesOf(add);
				preset->dataMap.remove(p);
				preset->addressMap.remove(add);
				preset->load(p);
				LOG("Removed " << p->niceName << " override from preset " << preset->niceName);
			}
			else
			{
				preset->save(p);
				LOG("Added " << p->niceName << " override to preset " << preset->niceName);
			}

		}
		return true;
	}

	return false;
}

LGMLMenuBarComponent::LGMLMenuBarComponent(MainComponent * mainComp, LGMLEngine * engine) :
	Component("LGML Menu Bar")
#if !JUCE_MAC
	, menuBarComp(mainComp)
#endif
{
#if !JUCE_MAC
	addAndMakeVisible(menuBarComp);
#endif

	cpuUsageUI.reset(engine->cpuUsage->createSlider());
	addAndMakeVisible(cpuUsageUI.get());

	logInUI.reset(OSCRemoteControl::getInstance()->logIncoming->createToggle());
	logOutUI.reset(OSCRemoteControl::getInstance()->logOutgoing->createToggle());

	addAndMakeVisible(logInUI.get());
	addAndMakeVisible(logOutUI.get());
}

LGMLMenuBarComponent::~LGMLMenuBarComponent()
{
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
	
}
