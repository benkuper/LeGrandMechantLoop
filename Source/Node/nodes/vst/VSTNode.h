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

class VSTNode :
	public Node,
	public MIDIInputDevice::MIDIInputListener,
	public AudioProcessorListener
{
public:
	VSTNode(var params = var());
	~VSTNode();

	VSTPluginParameter* pluginParam;
	ControllableContainer vstParamsCC;

	HashMap<int, Parameter*> idParamMap;
	HashMap<Parameter*, int> paramIDMap;
	HashMap<Parameter*, AudioProcessorParameter *> paramVSTMap;
	Array<int> antiFeedbackIDs;
	Array<Parameter*> antiFeedbackParams;

	MIDIDeviceParameter* midiParam;

	MIDIInputDevice* currentDevice;
	std::unique_ptr<AudioPluginInstance> vst;
	MidiMessageCollector midiCollector;


	bool isSettingVST; //avoid updating vst's playconfig while setting it

	void clearItem() override;

	void setMIDIDevice(MIDIInputDevice* d);
	void setupVST(PluginDescription* description);
	


	void rebuildVSTParameters();
	void fillContainerForVSTParamGroup(ControllableContainer * cc, const AudioProcessorParameterGroup* group);
	Parameter * createParameterForVSTParam(AudioProcessorParameter* vstParam);

	void updatePlayConfigInternal() override;

	void onContainerParameterChangedInternal(Parameter* p) override;
	void onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c) override;

	void midiMessageReceived(const MidiMessage& m) override;

	void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
	void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;
	
	String getTypeString() const override { return getTypeStringStatic(); }
	static const String getTypeStringStatic() { return "VST"; }
	
	BaseNodeViewUI* createViewUI() override;

	DECLARE_ASYNC_EVENT(VSTNode, VST, vst, ENUM_LIST(VST_REMOVED, VST_SET))

	// Inherited via AudioProcessorListener
	virtual void audioProcessorChanged(AudioProcessor* processor) override;
	virtual void audioProcessorParameterChanged(AudioProcessor* processor, int parameterIndex, float newValue) override;
	virtual void audioProcessorParameterChangeGestureBegin(AudioProcessor* processor, int paramIndex) override;
	virtual void audioProcessorParameterChangeGestureEnd(AudioProcessor* processor, int paramIndex) override;
};
