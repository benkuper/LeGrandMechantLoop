#include "MainComponent.h"

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
	ShapeShifterFactory::getInstance()->defs.add(new ShapeShifterDefinition("Interfaces", &InterfaceManagerUI::create));
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