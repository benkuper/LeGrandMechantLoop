#pragma once

#include <JuceHeader.h>
#include "Engine/AudioManager.h"

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/

ApplicationProperties& getAppProperties();
ApplicationCommandManager& getCommandManager();

class MainComponent;
class LGMLEngine;

class LGMLMenuBarComponent :
    public Component,
    public AudioManager::AudioManagerListener
{
public:
    LGMLMenuBarComponent(MainComponent * mainComp, LGMLEngine * engine);
    ~LGMLMenuBarComponent();

#if !JUCE_MAC
    MenuBarComponent menuBarComp;
#endif

    std::unique_ptr<FloatSliderUI> cpuUsageUI;
    std::unique_ptr<BoolToggleUI> logInUI;
    std::unique_ptr<BoolToggleUI> logOutUI;
    Label audioSettingsUI;

    void paint(Graphics& g) override;
    void resized() override;

    void audioSetupChanged() override;
};

class MainComponent : public OrganicMainContentComponent
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    void init() override;

    Outliner* createOutliner(const String& contentName) override;

    static void addControllableMenuItems(ControllableUI * ui, PopupMenu * p);
    static bool handleControllableMenuResult(ControllableUI* ui, int result);

private:
    //==============================================================================
    // Your private member variables go here...
    void getAllCommands(Array<CommandID>& commands) override;
    virtual void getCommandInfo(CommandID commandID, ApplicationCommandInfo& result) override;
    virtual bool perform(const InvocationInfo& info) override;
    StringArray getMenuBarNames() override;
    virtual PopupMenu getMenuForIndex(int topLevelMenuIndex, const String& menuName) override;
    void fillFileMenuInternal(PopupMenu& menu) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
