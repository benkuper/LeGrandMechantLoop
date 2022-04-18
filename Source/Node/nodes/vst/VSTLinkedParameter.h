/*
  ==============================================================================

    VSTLinkedParameter.h
    Created: 7 Dec 2020 4:54:15pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

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

    int maxMacros;

    void setMaxMacros(int val);

    void addAllParams();
    void removeAllParams();

    VSTParameterLink * addVSTParam(AudioProcessorParameter * p);
    void fillContainerForVSTParamGroup(ControllableContainer* cc, const AudioProcessorParameterGroup* group/*, bool createParameters = false*/);
    VSTParameterLink* createParameterForVSTParam(AudioProcessorParameter* vstParam);
    
    void inspectableDestroyed(Inspectable* i) override;

    bool hasParam(int index) { return idParamMap.contains(index); }

    var getJSONData() override;
    void loadJSONData(var data, bool createIfNotThere = false) override;

    InspectableEditor* getEditorInternal(bool isRoot, Array<Inspectable*> inspectables = {}) override;
};

class VSTParameterLink :
    public AudioProcessorParameter::Listener
{
public:
    VSTParameterLink(AudioProcessorParameter* vstParam, Parameter * parameter);
    virtual ~VSTParameterLink();
    
    AudioProcessorParameter* vstParam;
    Parameter* param;

    bool antiFeedback;
    float valueAtGestureStart;
    
    int macroIndex;
    int maxMacros;

    void setMaxMacros(int val);
    void setMacroIndex(int index);

    void updateFromVST(float value);
    void updateFromParam(float value);

    String getNameFromParam() const;
    String getDescriptionFromParam() const;

    virtual void parameterValueChanged(int parameterIndex, float newValue) override;
    virtual void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override;

    DECLARE_ASYNC_EVENT(VSTParameterLink, VSTParameterLink, vstParameterLink, { MACRO_UPDATED });

    InspectableEditor * getEditor(bool isRoot);
};


class VSTLinkedFloatParameter :
    public VSTParameterLink,
    public FloatParameter
{
public:
    VSTLinkedFloatParameter(AudioProcessorParameter * vstParam);
    ~VSTLinkedFloatParameter();

    void setValueInternal(var &value) override;
    InspectableEditor* getEditor(bool isRoot) { return VSTParameterLink::getEditor(isRoot); }

};

class VSTLinkedIntParameter :
    public VSTParameterLink,
    public IntParameter
{
public:
    VSTLinkedIntParameter(AudioProcessorParameter* vstParam);
    ~VSTLinkedIntParameter() {}

    void setValueInternal(var& value) override;
    InspectableEditor* getEditor(bool isRoot) { return VSTParameterLink::getEditor(isRoot); }

};

class VSTLinkedBoolParameter :
    public VSTParameterLink,
    public BoolParameter
{
public:
    VSTLinkedBoolParameter(AudioProcessorParameter* vstParam);
    ~VSTLinkedBoolParameter() {}
    
    void setValueInternal(var& value) override;
    InspectableEditor* getEditor(bool isRoot) { return VSTParameterLink::getEditor(isRoot); }

};