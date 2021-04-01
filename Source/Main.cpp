/*
  ==============================================================================

    This file contains the basic startup code for a JUCE application.

  ==============================================================================
*/

#include "Main.h"
#include "MainComponent.h"
#include "Engine/LGMLEngine.h"

LGMLApplication::LGMLApplication() :
	OrganicApplication(ProjectInfo::projectName,
		true, ImageCache::getFromMemory(BinaryData::icon_png, BinaryData::icon_pngSize))
{
}

void LGMLApplication::initialiseInternal(const String&)
{
	engine.reset(new LGMLEngine());
	mainComponent.reset(new MainComponent());

	//Call after engine init
	AppUpdater::getInstance()->setURLs("http://benjamin.kuperberg.fr/lgml/releases/update.json", "http://benjamin.kuperberg.fr/lgml/download/app/", "LGML");
	//HelpBox::getInstance()->helpURL = URL("http://benjamin.kuperberg.fr/lgml/help/");
	CrashDumpUploader::getInstance()->remoteURL = URL("http://benjamin.kuperberg.fr/lgml/support/crash_report.php");
	//CrashDumpUploader::getInstance()->crashImage = ImageCache::getFromMemory(BinaryData::crash_png, BinaryData::crash_pngSize);

	DashboardManager::getInstance()->setupDownloadURL("http://benjamin.kuperberg.fr/download/dashboard/dashboard.php");

    ShapeShifterManager::getInstance()->setDefaultFileData(BinaryData::default_lgmllayout);
	ShapeShifterManager::getInstance()->setLayoutInformations("lgmllayout", "LGML/layouts");
}

void LGMLApplication::afterInit()
{
	if (mainWindow != nullptr)
	{
		mainWindow->setMenuBarComponent(new LGMLMenuBarComponent((MainComponent*)mainComponent.get(), (LGMLEngine*)engine.get()));
	}
}
