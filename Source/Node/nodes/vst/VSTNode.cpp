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

VSTNode::VSTNode(var params) :
    Node(getTypeString(), params),
    vstParamsCC("VST Parameters"),
    currentDevice(nullptr),
    isSettingVST(false),
    vstNotifier(5)
{
    pluginParam = new VSTPluginParameter("VST", "The VST to use");
    ControllableContainer::addParameter(pluginParam);

    midiParam = new MIDIDeviceParameter("MIDI Device", true, false);
    ControllableContainer::addParameter(midiParam);

    addChildControllableContainer(&vstParamsCC);

    setAudioInputs(2);
    setAudioOutputs(2);
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
        vst->removeListener(this);
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

            rebuildVSTParameters();

            vst->addListener(this);
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

void VSTNode::rebuildVSTParameters()
{
    idParamMap.clear();
    paramIDMap.clear();
    paramVSTMap.clear();

    removeChildControllableContainer(&vstParamsCC); //do this to avoid awful
    vstParamsCC.clear();

    if (vst == nullptr) return;
    
    const AudioProcessorParameterGroup & paramTree = vst->getParameterTree();
    fillContainerForVSTParamGroup(&vstParamsCC, &paramTree);

    addChildControllableContainer(&vstParamsCC);
}

void VSTNode::fillContainerForVSTParamGroup(ControllableContainer* cc, const AudioProcessorParameterGroup* group)
{
    Array<AudioProcessorParameter*> params = group->getParameters(false);
    for (auto& vstP : params)
    {
        Parameter* pp = createParameterForVSTParam(vstP);
        idParamMap.set(vstP->getParameterIndex(), pp);
        paramIDMap.set(pp, vstP->getParameterIndex());
        paramVSTMap.set(pp, vstP);
        cc->addControllable(pp);
    }

    const Array<const AudioProcessorParameterGroup*> groups = group->getSubgroups(false);
    for (auto & g : groups)
    {
        ControllableContainer* childCC = new ControllableContainer(g->getName());
        fillContainerForVSTParamGroup(childCC, g);
        cc->addChildControllableContainer(childCC, true);
    }
}

Parameter * VSTNode::createParameterForVSTParam(AudioProcessorParameter* vstParam)
{
    Parameter* p = nullptr;

    String pName = vstParam->getName(32);
    String desc = pName + " (" + vstParam->getLabel() + ")";
    if (vstParam->isBoolean())
    {
        p = new BoolParameter(pName, desc, vstParam->getDefaultValue());
    }
    else if (vstParam->isDiscrete())
    {
        /*
        p = new EnumParameter(vstParam->getLabel(), "");
        StringArray options = pp->getAllValueStrings();
        for (auto& o : options) ((EnumParameter*)p)->addOption(o, vstParam->getValueForText(o), false);
        ((EnumParameter*)p)->setValue(vstParam->getCurrentValueAsText());
        */
        p = new IntParameter(pName, desc, vstParam->getDefaultValue(), 0);
        p->setValue(vstParam->getValue());
    }else
    {
        p = new FloatParameter(pName, desc, vstParam->getDefaultValue(), 0, 1);
        p->setValue(vstParam->getValue());
    }

    return p;
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

void VSTNode::onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c)
{
    if (cc == &vstParamsCC && vst != nullptr)
    {
        Parameter* p = (Parameter*)c;
        if (paramVSTMap.contains(p))
        {
            if (!antiFeedbackIDs.contains(paramIDMap[p]))
            {
                float val = p->getNormalizedValue();
                if (p->type == p->ENUM) val = ((EnumParameter*)p)->getValueData();
                paramVSTMap[p]->setValue(val);
            }
        }
    }

    Node::onControllableFeedbackUpdateInternal(cc, c);
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

BaseNodeViewUI* VSTNode::createViewUI()
{
    return new VSTNodeViewUI(this);
}


void VSTNode::audioProcessorChanged(AudioProcessor* processor)
{
    LOG("Audio processor changed");
}

void VSTNode::audioProcessorParameterChanged(AudioProcessor* processor, int parameterIndex, float newValue)
{
    if (idParamMap.contains(parameterIndex))
    {
        LOG("Param value changed : " << idParamMap[parameterIndex]->niceName << " > " << newValue);
        Parameter* p = idParamMap[parameterIndex];
        if (!antiFeedbackParams.contains(p))
        {
            antiFeedbackIDs.add(parameterIndex);
            if (p->type == p->ENUM) ((EnumParameter*)p)->setValueWithData(newValue);
            else p->setValue(newValue);
            antiFeedbackIDs.removeAllInstancesOf(parameterIndex);
        }
    }
    else
    {
        LOG("Param not found for index " << parameterIndex);
    }
}

void VSTNode::audioProcessorParameterChangeGestureBegin(AudioProcessor* processor, int parameterIndex)
{
    if (idParamMap.contains(parameterIndex))
    {
        LOG("Param gesture begin : " << idParamMap[parameterIndex]->niceName);
    }
    else
    {
        LOG("Param not found for index " << parameterIndex);
    }
}

void VSTNode::audioProcessorParameterChangeGestureEnd(AudioProcessor* processor, int parameterIndex)
{
    if (idParamMap.contains(parameterIndex))
    {
        LOG("Param gesture end : " << idParamMap[parameterIndex]->niceName);
    }
    else
    {
        LOG("Param not found for index " << parameterIndex);
    }
}
