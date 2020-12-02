/*
  ==============================================================================

    VSTNode.cpp
    Created: 15 Nov 2020 8:42:29am
    Author:  bkupe

  ==============================================================================
*/

#include "VSTNode.h"
#include "ui/VSTNodeViewUI.h"
#include "Transport/Transport.h"

VSTProcessor::VSTProcessor(Node* node) :
    GenericNodeProcessor(node),
    currentDevice(nullptr),
    vstNotifier(5)
{

    pluginParam = new VSTPluginParameter("VST", "The VST to use");
    ControllableContainer::addParameter(pluginParam);

    midiParam = new MIDIDeviceParameter("MIDI Device", true, false);
    ControllableContainer::addParameter(midiParam);

}

VSTProcessor::~VSTProcessor()
{
    setMIDIDevice(nullptr);
    setupVST(nullptr);
}

void VSTProcessor::setMIDIDevice(MIDIInputDevice* d)
{
    if (currentDevice == d) return;
    if (currentDevice != nullptr)
    {
        currentDevice->removeMIDIInputListener(this);
    }

    currentDevice = d;

    if (currentDevice != nullptr)
    {
        currentDevice->addMIDIInputListener(this);
    }
}

void VSTProcessor::setupVST(PluginDescription* description)
{
    bool shouldResume = !isSuspended();
    suspendProcessing(true);

    if (vst != nullptr)
    {
        vst->releaseResources();
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

            nodeRef->setMIDIIO(vst->acceptsMidi(), false);
        }
        else
        {
            nodeRef->setMIDIIO(false, false);
        }
    }

    if(shouldResume) suspendProcessing(false);
}

void VSTProcessor::updatePlayConfig()
{
    if(vst != nullptr) vst->setPlayConfigDetails(nodeRef->numInputs, nodeRef->numOutputs, getSampleRate(), getBlockSize());
    GenericNodeProcessor::updatePlayConfig();
}

void VSTProcessor::onContainerParameterChanged(Parameter* p)
{
    GenericNodeProcessor::onContainerParameterChanged(p);
    if (p == pluginParam) setupVST(pluginParam->getPluginDescription());
    if (p == midiParam) setMIDIDevice(midiParam->inputDevice);
}

void VSTProcessor::midiMessageReceived(const MidiMessage& m)
{
    midiCollector.addMessageToQueue(m); //ugly hack to have at least events sorted, but sampleNumber should be exact
}

void VSTProcessor::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock)
{
    if (vst != nullptr) vst->prepareToPlay(sampleRate, maximumExpectedSamplesPerBlock);
    midiCollector.reset(sampleRate);
}

void VSTProcessor::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    if (vst != nullptr)
    {
        if (currentDevice != nullptr)
        {
            inMidiBuffer.clear();
            //LOG(vst->getNumInputChannels() << ", " << vst->getNumOutputChannels() << " / " << buffer.getNumChannels());
            midiCollector.removeNextBlockOfMessages(inMidiBuffer, buffer.getNumSamples());
        }
       
        vst->setPlayHead(Transport::getInstance());
        vst->processBlock(buffer, inMidiBuffer);
    }

    inMidiBuffer.clear();
}

NodeViewUI* VSTProcessor::createNodeViewUI()
{
    return new VSTNodeViewUI((GenericNode<VSTProcessor>*)nodeRef.get());
}
