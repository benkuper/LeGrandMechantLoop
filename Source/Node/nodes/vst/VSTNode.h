/*
  ==============================================================================

    VSTNode.h
    Created: 15 Nov 2020 8:42:29am
    Author:  bkupe

  ==============================================================================
*/

#pragma once
#include "../../NodeAudioProcessor.h"
#include "Engine/VSTManager.h"

class VSTProcessor :
	public GenericNodeAudioProcessor
{
public:
	VSTProcessor(Node* node);
	~VSTProcessor() {}

	VSTPluginParameter* pluginParam;
	Array<Parameter *> VSTParameters;

	std::unique_ptr<AudioPluginInstance> vst;

	void setupVST(PluginDescription* description);
	void updatePlayConfig() override;

	void onContainerParameterChanged(Parameter* p) override;

	void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock);
	void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;
	static const String getTypeStringStatic() { return "VST"; }
	NodeViewUI* createNodeViewUI() override;

	DECLARE_ASYNC_EVENT(VSTProcessor, VST, vst, ENUM_LIST(VST_REMOVED, VST_SET))
};
