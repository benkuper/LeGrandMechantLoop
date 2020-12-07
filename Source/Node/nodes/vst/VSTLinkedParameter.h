/*
  ==============================================================================

    VSTLinkedParameter.h
    Created: 7 Dec 2020 4:54:15pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"

class VSTParameterLink;

class VSTParameterContainer :
    public ControllableContainer,
    public Inspectable::InspectableListener
{
public:
    VSTParameterContainer(AudioPluginInstance * vst);
    ~VSTParameterContainer();

    AudioPluginInstance* vst;

    HashMap<int, ControllableContainer*> idContainerMap;
    HashMap<int,  VSTParameterLink *> idParamMap;
    HashMap<Inspectable*, int> inspectableIdMap;

    void addAllParams();
    void removeAllParams();

    VSTParameterLink * addVSTParam(AudioProcessorParameter * p);
    void fillContainerForVSTParamGroup(ControllableContainer* cc, const AudioProcessorParameterGroup* group/*, bool createParameters = false*/);
    Parameter* createParameterForVSTParam(AudioProcessorParameter* vstParam);

    void inspectableDestroyed(Inspectable* i) override;

    bool hasParam(int index) { return idParamMap.contains(index); }

    var getJSONData() override;
    void loadJSONData(var data, bool createIfNotThere = false) override;

    InspectableEditor* getEditor(bool isRoot) override;
};

class VSTParameterLink :
    public AudioProcessorParameter::Listener
{
public:
    VSTParameterLink(AudioProcessorParameter* vstParam,Parameter * parameter);
    virtual ~VSTParameterLink();
    
    AudioProcessorParameter* vstParam;
    Parameter* param;

    bool antiFeedback;
    float valueAtGestureStart;

    void updateFromVST(float value);
    void updateFromParam(float value);

    String getNameFromParam() const;
    String getDescriptionFromParam() const;

    // Inherited via Listener
    virtual void parameterValueChanged(int parameterIndex, float newValue) override;
    virtual void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override;
};


class VSTLinkedFloatParameter :
    public VSTParameterLink,
    public FloatParameter
{
public:
    VSTLinkedFloatParameter(AudioProcessorParameter * vstParam);
    ~VSTLinkedFloatParameter();

    void setValueInternal(var &value) override;
};

class VSTLinkedIntParameter :
    public VSTParameterLink,
    public IntParameter
{
public:
    VSTLinkedIntParameter(AudioProcessorParameter* vstParam);
    ~VSTLinkedIntParameter() {}

    void setValueInternal(var& value) override;
};

class VSTLinkedBoolParameter :
    public VSTParameterLink,
    public BoolParameter
{
public:
    VSTLinkedBoolParameter(AudioProcessorParameter* vstParam);
    ~VSTLinkedBoolParameter() {}

    void setValueInternal(var& value) override;
};