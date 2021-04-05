#include "MainComponent.h"

#include "Engine/LGMLEngine.h"

#include  "Node/ui/NodeManagerUI.h"
#include  "Node/ui/NodeManagerViewUI.h"
#include "Interface/ui/InterfaceManagerUI.h"
#include "Transport/ui/TransportUI.h"

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

	ShapeShifterFactory::getInstance()->defs.add(new ShapeShifterDefinition("Nodes (List View)", &NodeManagerPanel::create));
	ShapeShifterFactory::getInstance()->defs.add(new ShapeShifterDefinition("Nodes (2D View)", &NodeManagerViewPanel::create));
	//ShapeShifterFactory::getInstance()->defs.add(new ShapeShifterDefinition("Interfaces", &InterfaceManagerUI::create));
	ShapeShifterFactory::getInstance()->defs.add(new ShapeShifterDefinition("Transport", &TransportUI::create));
	//ShapeShifterFactory::getInstance()->defs.add(new ShapeShifterDefinition("Timeline", &TimelineU::create));


	OrganicMainContentComponent::init();

	String lastVersion = getAppProperties().getUserSettings()->getValue("lastVersion", "");

	if (lastVersion != getAppVersion())
	{
		//WelcomeScreen w;
		//DialogWindow::showModalDialog("Welcome", &w, getTopLevelComponent(), Colours::black, true);
	}
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
}

LGMLMenuBarComponent::~LGMLMenuBarComponent()
{
}

void LGMLMenuBarComponent::resized()
{
	Rectangle<int> r = getLocalBounds();
#if !JUCE_MAC	
	menuBarComp.setBounds(r); 
#endif
	cpuUsageUI->setBounds(r.removeFromRight(200).reduced(2)); //overlap but we don't care
	
}
