#pragma once

#include <JuceHeader.h>

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
    public Component
{
public:
    LGMLMenuBarComponent(MainComponent * mainComp, LGMLEngine * engine);
    ~LGMLMenuBarComponent();

    MenuBarComponent menuBarComp;
    std::unique_ptr<FloatSliderUI> cpuUsageUI;

    void resized() override;
};

class MainComponent : public OrganicMainContentComponent
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    void init() override;

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
