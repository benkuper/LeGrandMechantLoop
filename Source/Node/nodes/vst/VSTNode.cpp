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
#include "VSTLinkedParameter.h"

VSTNode::VSTNode(var params) :
    Node(getTypeString(), params),
    currentDevice(nullptr),
    isSettingVST(false),
    vstNotifier(5)
{
    pluginParam = new VSTPluginParameter("VST", "The VST to use");
    ControllableContainer::addParameter(pluginParam);

    midiParam = new MIDIDeviceParameter("MIDI Device", true, false);
    ControllableContainer::addParameter(midiParam);

    setAudioInputs(2);
    setAudioOutputs(2);

    viewUISize->setPoint(200, 150);
}

VSTNode::~VSTNode()
{
    
}

void VSTNode::clearItem()
{
    Node::clearItem();
    setMIDIDevice(nullptr);
    setupVST(nullptr);
}

void VSTNode::setMIDIDevice(MIDIInputDevice* d)
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

void VSTNode::setupVST(PluginDescription* description)
{
    ScopedSuspender sp(processor);

    isSettingVST = true;


    if (vst != nullptr)
    {
        vst->releaseResources();
    }

    if (vstParamsCC != nullptr)
    {
        removeChildControllableContainer(vstParamsCC.get());
        vstParamsCC.reset();
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
        vst = VSTManager::getInstance()->formatManager.createPluginInstance(*description, processor->getSampleRate(), processor->getBlockSize(), errorMessage);

        if (errorMessage.isNotEmpty())
        {
            NLOGERROR(niceName, "VST Load error : " << errorMessage);
        }

        if (vst != nullptr)
        {
            vst->setPlayHead(Transport::getInstance());
            setAudioInputs(description->numInputChannels);
            setAudioOutputs(description->numOutputChannels); 
            setMIDIIO(vst->acceptsMidi(), false);

            vstParamsCC.reset(new VSTParameterContainer(vst.get()));
            addChildControllableContainer(vstParamsCC.get());
        }
        else
        {
            setMIDIIO(false, false);
        }
    }

    isSettingVST = false;
    updatePlayConfig();

    vstNotifier.addMessage(new VSTEvent(VSTEvent::VST_SET, this));
}

void VSTNode::updatePlayConfigInternal()
{
    Node::updatePlayConfigInternal();

    if (vst != nullptr && !isSettingVST)
    {
        vst->setRateAndBufferSizeDetails(processor->getSampleRate(), processor->getBlockSize());
        vst->prepareToPlay(processor->getSampleRate(), processor->getBlockSize());
    }
}

void VSTNode::onContainerParameterChangedInternal(Parameter* p)
{
    Node::onContainerParameterChangedInternal(p);
    if (p == pluginParam) setupVST(pluginParam->getPluginDescription());
    if (p == midiParam) setMIDIDevice(midiParam->inputDevice);
}


void VSTNode::midiMessageReceived(const MidiMessage& m)
{
    midiCollector.addMessageToQueue(m); //ugly hack to have at least events sorted, but sampleNumber should be exact
}

void VSTNode::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock)
{
    if (vst != nullptr) vst->prepareToPlay(sampleRate, maximumExpectedSamplesPerBlock);
    if(sampleRate != 0) midiCollector.reset(sampleRate);
}

void VSTNode::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    if (vst != nullptr)
    {
        if (currentDevice != nullptr)
        {
            inMidiBuffer.clear();
            midiCollector.removeNextBlockOfMessages(inMidiBuffer, buffer.getNumSamples());
        }
       
        vst->processBlock(buffer, inMidiBuffer);
    }

    inMidiBuffer.clear();
}

void VSTNode::processBlockBypassed(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    buffer.clear();
}

var VSTNode::getJSONData()
{
    var data = Node::getJSONData();
    if (vst != nullptr)
    {
        MemoryBlock b;
        vst->getStateInformation(b);
        data.getDynamicObject()->setProperty("vstState", b.toBase64Encoding());
    }

    if (vstParamsCC != nullptr) data.getDynamicObject()->setProperty("vstParams", vstParamsCC->getJSONData());
    return data;
}

void VSTNode::loadJSONDataItemInternal(var data)
{
    Node::loadJSONDataItemInternal(data);

    if (vst != nullptr)
    {
        String vstData = data.getProperty("vstState", "");
        MemoryBlock b;
        if (b.fromBase64Encoding(vstData)) vst->setStateInformation(b.getData(), b.getSize());
    }

    if(vstParamsCC != nullptr) vstParamsCC->loadJSONData(data.getProperty("vstParams", var()));
}

BaseNodeViewUI* VSTNode::createViewUI()
{
    return new VSTNodeViewUI(this);
}
