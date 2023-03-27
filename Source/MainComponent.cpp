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
	ShapeShifterFactory::getInstance()->defs.add(new ShapeShifterDefinition("Macros", &MacroManagerUI::create));
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
	if (ui->controllable->type == Controllable::TRIGGER && !ui->controllable->isControllableFeedbackOnly) return;

	Parameter* param = (Parameter*)ui->controllable.get();
	bool isPresettable = RootPresetManager::getInstance()->isParameterPresettable(param);
	p->addItem(0x5000, "Presettable", true, isPresettable);

	if (isPresettable)
	{
		Preset* preset = RootPresetManager::getInstance()->currentPreset;
		String presetName = preset != nullptr ? preset->niceName : "No loaded preset";
		String add = ui->controllable->getControlAddress();
		bool canBeOverride = preset != nullptr && !preset->isMain();
		bool isOverride = canBeOverride && preset->overridenControllables.contains(add);


		bool canInterpolate = param->type != Parameter::BOOL && param->type != Parameter::ENUM && param->type != Parameter::STRING && param->type != Parameter::ENUM;
		int option = RootPresetManager::getInstance()->getParameterPresetOption(param, "transition", (int)(canInterpolate ? Preset::INTERPOLATE : Preset::DEFAULT));
		PopupMenu interpMenu;
		interpMenu.addItem(0x5010, "Interpolate", canInterpolate, option == Preset::INTERPOLATE);
		interpMenu.addItem(0x5013, "Default", !canInterpolate, option == Preset::DEFAULT);

		interpMenu.addItem(0x5011, "Change at start", true, option == Preset::AT_START);
		interpMenu.addItem(0x5012, "Chante at end", true, option == Preset::AT_END);
		p->addSubMenu("Transition Mode", interpMenu);

		p->addItem(0x5001, "Save to current (" + presetName + ")", true, isOverride);

		PopupMenu saveToOtherMenu;
		RootPresetManager::getInstance()->fillPresetMenu(saveToOtherMenu, 0x20000, param);
		p->addSubMenu("Save to...", saveToOtherMenu);

		p->addItem(0x5002, "Remove from current (" + presetName + ")", isOverride);
		PopupMenu removeFromMenu;
		RootPresetManager::getInstance()->fillPresetMenu(removeFromMenu, 0x30000, param);
		p->addSubMenu("Remove from...", removeFromMenu);
	}
}

bool MainComponent::handleControllableMenuResult(ControllableUI* ui, int result)
{
	if (result < 0x5000 && result > 0x5020) return false;
	if (ui->controllable->type == Controllable::TRIGGER) return true;

	Parameter* p = (Parameter*)ui->controllable.get();

	if (result == 0x5000)
	{
		RootPresetManager::getInstance()->toggleParameterPresettable(p);
		return true;
	}
	else if (result == 0x5001 || result == 0x5002)
	{
		if (Preset* preset = RootPresetManager::getInstance()->currentPreset)
		{
			String add = p->getControlAddress();
			if (result == 0x5001)
			{
				preset->save(p);
				LOG("Saved " << p->niceName << " to preset " << preset->niceName << (preset->isMain() ? "" : " (Override)"));
			}
			else if (result == 0x5002)
			{
				preset->removeParameterFromDataMap(p);
				preset->load(p);
				LOG("Removed " << p->niceName << " from preset " << preset->niceName << (preset->isMain() ? "" : " (Override)"));
			}

		}
		return true;
	}
	else if (result >= 0x5010 && result <= 0x5013)
	{
		int option = result - 0x5010;
		RootPresetManager::getInstance()->setParameterPresetOption(p, "transition", option);
	}
	else if (result >= 0x20000 && result < 0x30000)
	{
		if (Preset* preset = RootPresetManager::getInstance()->getPresetForMenuResult(result - 0x20000)) preset->save(p);
	}
	else if (result >= 0x30000)
	{
		if (Preset* preset = RootPresetManager::getInstance()->getPresetForMenuResult(result - 0x30000)) preset->removeParameterFromDataMap(p);
	}

	return false;
}

LGMLMenuBarComponent::LGMLMenuBarComponent(MainComponent* mainComp, LGMLEngine* engine) :
	Component("LGML Menu Bar")
#if !JUCE_MAC
	, menuBarComp(mainComp)
#endif
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
