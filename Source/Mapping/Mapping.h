/*
  ==============================================================================

    Mapping.h
    Created: 3 May 2021 10:23:49am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"
#include "Common/CommonIncludes.h"
#include "Interface/InterfaceIncludes.h"

class Mapping :
    public BaseItem
{
public:
    Mapping(var params = var());
    virtual ~Mapping();

    TargetParameter* destParam;

    Point2DParameter* inputRange;
    Point2DParameter* outputRange;

    enum OutsideBehaviour { CLIP, LOOP, ZERO };
    EnumParameter* outsideBehaviour;

    enum BoolBehaviour { NONZERO, ONE, DEFAULT };
    EnumParameter* boolBehaviour;
    BoolParameter* boolToggle;
    bool prevVal; //for toggle

    BoolParameter* autoFeedback;
    bool isProcessing;
    bool isSendingFeedback;

    WeakReference<Controllable> dest;

    void setDest(Controllable* c);

    virtual void onContainerParameterChangedInternal(Parameter*) override;
    virtual void onExternalParameterValueChanged(Parameter*) override;


    virtual void sendFeedback() {}

    virtual void process(var value);
};

class GenericMapping :
    public Mapping
{
public:
    GenericMapping(var params = var());
    ~GenericMapping();

    TargetParameter* sourceParam;
    WeakReference<Controllable> source;

    void setSource(Controllable* p);

    void onContainerParameterChangedInternal(Parameter* p) override;
    void onExternalParameterValueChanged(Parameter* p) override;
    void onExternalTriggerTriggered(Trigger* t) override;

    DECLARE_TYPE("Generic")
};

class MacroMapping :
    public Mapping
{
public:
    MacroMapping(var params = var());
    ~MacroMapping();

    FloatParameter * sourceParam;

    void onContainerParameterChangedInternal(Parameter* p) override;

    DECLARE_TYPE("Macro");
};

class MIDIMapping :
    public Mapping,
    public MIDIInterface::MIDIInterfaceListener
{
public:
    MIDIMapping(var params = var());
    ~MIDIMapping();

    void clearItem() override;

    TargetParameter* interfaceParam;
    MIDIInterface* midiInterface;
    IntParameter* channel;

    enum MIDIType { NOTE, CC, PitchWheel };
    EnumParameter * type;
    IntParameter* pitchOrNumber;
    BoolParameter* learn;

    void setMIDIInterface(MIDIInterface* d);
    void onContainerParameterChangedInternal(Parameter* p) override;

    void sendFeedback() override;

    void noteOnReceived(MIDIInterface*, const int& channel, const int& pitch, const int& velocity) override;
    void noteOffReceived(MIDIInterface*, const int& channel, const int& pitch, const int& velocity) override;
    void controlChangeReceived(MIDIInterface*, const int& channel, const int& number, const int& value) override;
    void pitchWheelReceived(MIDIInterface*, const int& channel, const int &value) override;

    DECLARE_TYPE("MIDI");

};