/*
  ==============================================================================

	VSTManager.h
	Created: 24 Nov 2020 9:39:06am
	Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"

class VSTManager :
	public ControllableContainer,
	public Thread
{
public:
	juce_DeclareSingleton(VSTManager, true)

		VSTManager();
	~VSTManager();

	std::unique_ptr<AudioPluginFormatManager> formatManager;
	OwnedArray<PluginDescription> descriptions;
	HashMap<String, PluginDescription*> idDescriptionMap;
	HashMap<int, PluginDescription*> uidDescriptionMap;

	Trigger* rescan;
	BoolParameter* scanVST;
	BoolParameter* scanAU;
	BoolParameter* scanLV2;

	void updateVSTFormats();
	void updateVSTList();
	void onContainerParameterChanged(Parameter* p) override;
	void onControllableAdded(Controllable* c) override;
	void onControllableRemoved(Controllable* c) override;

	void reset();

	var getJSONData() override;
	void loadJSONDataInternal(var data) override;
	void afterLoadJSONDataInternal() override;

	void onContainerTriggerTriggered(Trigger*) override;

	String getParameterValueForDescription(PluginDescription* d);

	void run() override;

	DECLARE_ASYNC_EVENT(VSTManager, VSTManager, vstManager, ENUM_LIST(PLUGINS_UPDATED))


		InspectableEditor* getEditorInternal(bool isRoot, Array<Inspectable*> inspectables = {}) override;
};

class VSTPluginParameterUI;

class VSTPluginParameter :
	public StringParameter
{
public:
	VSTPluginParameter(const String& name, const String& description);
	~VSTPluginParameter();

	int vstID;
	PluginDescription* getPluginDescription();

	void setValueInternal(var& value) override;

	VSTPluginParameterUI* createVSTParamUI();
	ControllableUI* createDefaultUI(Array<Controllable*> controllables = {}) override;

	var getJSONDataInternal();
	void loadJSONDataInternal(var data);
};

class DescriptionSorter
{
public:
	static int compareElements(PluginDescription* first, PluginDescription* second);
};
