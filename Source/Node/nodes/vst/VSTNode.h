/*
  ==============================================================================

    VSTNode.h
    Created: 15 Nov 2020 8:42:29am
    Author:  bkupe

  ==============================================================================
*/

#pragma once
#include "../../NodeProcessor.h"
#include "Engine/VSTManager.h"
#include "Common/MIDI/MIDIDeviceParameter.h"

class VSTProcessor :
	public GenericNodeProcessor,
	public MIDIInputDevice::MIDIInputListener
{
public:
	VSTProcessor(Node* node);
	~VSTProcessor();

	VSTPluginParameter* pluginParam;
	Array<Parameter *> VSTParameters;
	MIDIDeviceParameter* midiParam;

	MIDIInputDevice* currentDevice;
	std::unique_ptr<AudioPluginInstance> vst;
	MidiMessageCollector midiCollector;

	void setMIDIDevice(MIDIInputDevice* d);
	void setupVST(PluginDescription* description);
	void updatePlayConfig() override;

	void onContainerParameterChanged(Parameter* p) override;

	void midiMessageReceived(const MidiMessage& m) override;

	void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
	void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;
	static const String getTypeStringStatic() { return "VST"; }
	NodeViewUI* createNodeViewUI() override;

	DECLARE_ASYNC_EVENT(VSTProcessor, VST, vst, ENUM_LIST(VST_REMOVED, VST_SET))
};
