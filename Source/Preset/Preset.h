/*
  ==============================================================================

	Preset.h
	Created: 17 Apr 2021 12:08:52pm
	Author:  bkupe

  ==============================================================================
*/

#pragma once

class PresetManager;

class Preset :
	public BaseItem,
	public BaseManager<Preset>::ManagerListener,
	public EngineListener
{
public:
	Preset(var params = var());
	~Preset();

	BoolParameter* isCurrent;
	StringParameter* description;

	Trigger* saveTrigger;
	Trigger* loadTrigger;

	BoolParameter* skipInPrev;
	BoolParameter* skipInNext;
	BoolParameter* noParentOnNearbyLoad;

	EnablingControllableContainer transitionCC;
	EnumParameter* transitionQuantiz;
	IntParameter* numBeatBarQuantiz;
	FloatParameter* transitionTime;
	enum TransitionMode { INTERPOLATE, AT_START, AT_END, DEFAULT };
	EnumParameter* defaultTransitionMode;
	Automation transitionCurve;

	//overriding
	HashMap<WeakReference<Controllable>, var> dataMap;
	HashMap<WeakReference<Controllable>, TransitionMode> transitionMap;
	HashMap<WeakReference<Controllable>, String> controllableGhostAddressMap;
	HashMap<String, TransitionMode> lostControllables;
	HashMap<String, var> addressMap;
	Array<String> overridenControllables;
	Array<WeakReference<Controllable>> ignoredControllables;

	std::unique_ptr<PresetManager> subPresets;

	ControllableContainer linkedPresetsCC;

	void clearItem() override;

	var getPresetValues(bool includeParents = true, Array<Controllable*> ignoreList = Array<Controllable*>(), bool force = false);

	void saveContainer(ControllableContainer* container, bool recursive);
	void save(Controllable* controllable = nullptr, bool saveAllPresettables = false, bool noCheck = false);
	void load(bool recursive = false);

	void addControllableToDataMap(Controllable* c, var forceValue = var());
	void updateControllableAddress(Controllable* c);
	void removeControllableFromDataMap(Controllable* c);
	void removeAddressFromDataMap(String address);

	void recoverLostControllables();

	bool isMain(); //check if it's not an override
	bool hasPresetControllable(Controllable* c);



	void onContainerTriggerTriggered(Trigger* t) override;
	void onControllableFeedbackUpdate(ControllableContainer* cc, Controllable* c) override;

	void itemAdded(Preset* p) override;

	void controllableAdded(Controllable* c); //for linked presets


	var getJSONData() override;
	void loadJSONDataItemInternal(var data) override;

	void controllableControlAddressChanged(Controllable* c) override;

	void childStructureChanged(ControllableContainer* cc) override;
	void endLoadFile() override;

	InspectableEditor* getEditorInternal(bool isRoot, Array<Inspectable*> controllables = {}) override;

	DECLARE_TYPE("Preset");
};