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
	public Node
{
public:
	VSTNode(var params = var());
	~VSTNode();

	VSTPluginParameter* pluginParam;
	std::unique_ptr<VSTParameterContainer> vstParamsCC;
	ControllableContainer macrosCC;
	IntParameter * numMacros;
	IntParameter* autoActivateMacroIndex;
	Point2DParameter* autoActivateRange;

	struct VSTPreset
	{
		String name;
		String data;
	};

	OwnedArray<VSTPreset> presets;
	VSTPreset* currentPreset;

	EnumParameter* presetEnum;
	std::unique_ptr<AudioPluginInstance> vst;

	SpinLock vstStateLock;

	bool antiMacroFeedback;
	bool isSettingVST; //avoid updating vst's playconfig while setting it

	void clearItem() override;
	
	void setupVST(PluginDescription* description);
	void setIOFromVST();

	String getVSTState();
	void setVSTState(const String& data);

	void updatePresetEnum(const String & setPresetName = "");
	void updateMacros();

	void checkAutoBypassFromMacro();

	void updatePlayConfigInternal() override;

	void onContainerParameterChangedInternal(Parameter* p) override;
	void onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c) override;
	void onControllableStateChanged(Controllable* c) override;


	void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
	void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;
	void processBlockBypassed(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;
	void processVSTBlock(AudioBuffer<float>& buffer, MidiBuffer &midiMessages, bool bypassed);

	var getJSONData() override;
	void loadJSONDataItemInternal(var data) override;

	String getTypeString() const override { return getTypeStringStatic(); }
	static const String getTypeStringStatic() { return "VST"; }

	BaseNodeViewUI* createViewUI() override;

	DECLARE_ASYNC_EVENT(VSTNode, VST, vst, ENUM_LIST(VST_REMOVED, VST_SET))
};
