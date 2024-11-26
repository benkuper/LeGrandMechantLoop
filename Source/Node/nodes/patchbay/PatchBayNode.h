/*
  ==============================================================================

	PatchBayNode.h
	Created: 26 Nov 2024 12:04:38pm
	Author:  bkupe

  ==============================================================================
*/

#pragma once

class PatchConnection :
	public NodeConnection
{
public:
	PatchConnection(var params = var());
	~PatchConnection() {}

	EnumParameter* input;
	EnumParameter* output;

	String ghostInput;
	String ghostOutput;

	void updateInputOptions(StringArray options, const String& forceNewVal = "");
	void updateOutputOptions(StringArray options, const String& forceNewVal = "");

	DECLARE_TYPE("PatchConnection")
};

class PatchBayNode :
	public Node,
	public BaseManager<PatchConnection>::ManagerListener
{
public:
	PatchBayNode(var params = var());
	~PatchBayNode() {}

	BaseManager<PatchConnection> connections;

	ControllableContainer presetsCC;
	EnumParameter* presetEnum;
	StringParameter* newPresetName;
	Trigger* addPreset;
	Trigger* savePreset;
	Trigger* deletePreset;
	Trigger* reloadPreset;
	Trigger* updatePresetName;

	var presets;

	var connectionsData; //to load after input output names have been set

	void updatePresetEnum();
	void loadCurrentPreset();

	void itemAdded(PatchConnection* item) override;
	void itemsAdded(Array<PatchConnection*>) override;
	void itemRemoved(PatchConnection* item) override;

	void onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c) override;
	void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

	var getJSONData() override;
	void loadJSONDataItemInternal(var data) override;
	void afterLoadJSONDataInternal() override;

	DECLARE_TYPE("Patch Bay")
};