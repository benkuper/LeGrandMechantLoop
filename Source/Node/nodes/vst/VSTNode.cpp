/*
  ==============================================================================

    VSTNode.cpp
    Created: 15 Nov 2020 8:42:29am
    Author:  bkupe

  ==============================================================================
*/

#include "VSTNode.h"
#include "ui/VSTNodeViewUI.h"

VSTProcessor::VSTProcessor(Node* node) :
    GenericNodeAudioProcessor(node),
    vstNotifier(5)
{
    pluginParam = new VSTPluginParameter("VST", "The VST to use");
    ControllableContainer::addParameter(pluginParam);
}

void VSTProcessor::setupVST(PluginDescription* description)
{
    bool shouldResume = !isSuspended();
    suspendProcessing(true);

    if (vst != nullptr)
    {

    }

    vstNotifier.addMessage(new VSTEvent(VSTEvent::VST_REMOVED, this));
    vstNotifier.triggerAsyncUpdate();
    
    if (description == nullptr)
    {
        vst.reset();
    }
    else
    {

        String errorMessage;
        vst = VSTManager::getInstance()->formatManager.createPluginInstance(*description, getSampleRate(), getBlockSize(), errorMessage);

        if (errorMessage.isNotEmpty())
        {
            NLOGERROR(niceName, "VST Load error : " << errorMessage);
        }

        if (vst != nullptr)
        {
            nodeRef->setAudioInputs(description->numInputChannels);
            nodeRef->setAudioOutputs(description->numOutputChannels);
            vst->prepareToPlay(getSampleRate(), getBlockSize());
            vstNotifier.addMessage(new VSTEvent(VSTEvent::VST_SET, this));
            DBG("VST IS READY");
        }
    }

    if(shouldResume) suspendProcessing(false);
}

void VSTProcessor::updatePlayConfig()
{
    if(vst != nullptr) vst->setPlayConfigDetails(nodeRef->numInputs, nodeRef->numOutputs, getSampleRate(), getBlockSize());
    GenericNodeAudioProcessor::updatePlayConfig();
}

void VSTProcessor::onContainerParameterChanged(Parameter* p)
{
    GenericNodeAudioProcessor::onContainerParameterChanged(p);
    if (p == pluginParam) setupVST(pluginParam->getPluginDescription());
}

void VSTProcessor::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock)
{
    if (vst != nullptr) vst->prepareToPlay(sampleRate, maximumExpectedSamplesPerBlock);
}

void VSTProcessor::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    if (vst != nullptr)
    {
        //LOG(vst->getNumInputChannels() << ", " << vst->getNumOutputChannels() << " / " << buffer.getNumChannels());
        DBG("Process");
        vst->processBlock(buffer, midiMessages);
    }
}

NodeViewUI* VSTProcessor::createNodeViewUI()
{
    return new VSTNodeViewUI((GenericAudioNode<VSTProcessor>*)nodeRef.get());
}
