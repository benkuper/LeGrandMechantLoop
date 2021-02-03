/*
  ==============================================================================

    VSTNode.h
    Created: 15 Nov 2020 8:42:29am
    Author:  bkupe

  ==============================================================================
*/

#pragma once
#include "../../Node.h"
#include "Engine/VSTManager.h"
#include "Common/MIDI/MIDIDeviceParameter.h"

class VSTParameterContainer;

class VSTNode :
	public Node,
	public MIDIInputDevice::MIDIInputListener
{
public:
	VSTNode(var params = var());
	~VSTNode();

	VSTPluginParameter* pluginParam;
	std::unique_ptr<VSTParameterContainer> vstParamsCC;

	MIDIDeviceParameter* midiParam;

	MIDIInputDevice* currentDevice;
	std::unique_ptr<AudioPluginInstance> vst;
	MidiMessageCollector midiCollector;

	bool isSettingVST; //avoid updating vst's playconfig while setting it

	void clearItem() override;

	void setMIDIDevice(MIDIInputDevice* d);
	void setupVST(PluginDescription* description);
	
	void updatePlayConfigInternal() override;

	void onContainerParameterChangedInternal(Parameter* p) override;

	void midiMessageReceived(const MidiMessage& m) override;

	void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
	void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;
	void processBlockBypassed(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

	var getJSONData() override;
	void loadJSONDataItemInternal(var data) override;

	String getTypeString() const override { return getTypeStringStatic(); }
	static const String getTypeStringStatic() { return "VST"; }
	
	BaseNodeViewUI* createViewUI() override;

	DECLARE_ASYNC_EVENT(VSTNode, VST, vst, ENUM_LIST(VST_REMOVED, VST_SET))
};
