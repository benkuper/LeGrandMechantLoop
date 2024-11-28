/*
  ==============================================================================

	VSTNode.h
	Created: 15 Nov 2020 8:42:29am
	Author:  bkupe

  ==============================================================================
*/

#pragma once

class VSTParameterContainer;

class VSTNode :
	public Node,
	public AudioManager::AudioManagerListener
{
public:
	VSTNode(var params = var());
	~VSTNode();

	VSTPluginParameter* pluginParam;
	BoolParameter* clearBufferOnDisable;
	FloatParameter* dryWet;
	BoolParameter* disableOnDry;
	std::unique_ptr<VSTParameterContainer> vstParamsCC;
	ControllableContainer macrosCC;
	IntParameter * numMacros;
	IntParameter* autoActivateMacroIndex;
	Point2DParameter* autoActivateRange;

	BoolParameter* showPluginParam;
	BoolParameter* showMidiDevice;
	BoolParameter* showMacros;


	struct VSTPreset
	{
		String name;
		String data;
	};

	OwnedArray<VSTPreset> presets;
	VSTPreset* currentPreset;

	EnumParameter* presetEnum;
	ControllableContainer presetsCC;
	StringParameter* newPresetName;
	Trigger* addPreset;
	Trigger* savePreset;
	Trigger* deletePreset;
	Trigger* reloadPreset;
	Trigger* updatePresetName;

	std::unique_ptr<AudioPluginInstance> vst;

	SpinLock vstStateLock;

	bool antiMacroFeedback;
	bool isSettingVST; //avoid updating vst's playconfig while setting it
	float prevWetDry;

	void clearItem() override;
	
	void setupVST(PluginDescription* description);
	void setIOFromVST();

	String getVSTState();
	void setVSTState(const String& data);

	void updatePresetEnum(const String & setPresetName = "");
	void createNewPreset(const String& presetName);
	String getUniquePresetName(const String& name);
	void updateMacros();

	void checkAutoBypass();

	void updatePlayConfigInternal() override;

	void audioSetupChanged() override;

	void onContainerParameterChangedInternal(Parameter* p) override;
	void onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c) override;
	void onControllableStateChanged(Controllable* c) override;


	void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
	void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;
	void processBlockBypassed(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;
	void processVSTBlock(AudioBuffer<float>& buffer, MidiBuffer &midiMessages, bool bypassed);

	void bypassInternal() override;

	var getJSONData() override;
	void loadJSONDataItemInternal(var data) override;

	DECLARE_TYPE("VST")

	BaseNodeViewUI* createViewUI() override;

	DECLARE_ASYNC_EVENT(VSTNode, VST, vst, ENUM_LIST(VST_REMOVED, VST_SET), EVENT_ITEM_CHECK)
};
