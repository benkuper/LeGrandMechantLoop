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

class Mapping :
    public BaseItem
{
public:
    Mapping(var params = var());
    virtual ~Mapping();

    TargetParameter * destParam;

    Point2DParameter* inputRange;
    Point2DParameter* outputRange;

    enum OutsideBehaviour { CLIP, LOOP, ZERO };
    EnumParameter* outsideBehaviour;

    enum BoolBehaviour { NONZERO, ONE, DEFAULT };
    EnumParameter* boolBehaviour;
    BoolParameter* boolToggle;
    bool prevVal; //for toggle

    WeakReference<Controllable> dest;

    void setDest(Controllable* c);

    virtual void onContainerParameterChangedInternal(Parameter*) override;

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

    String getTypeString() const override { return "Generic"; }
};

class MacroMapping :
    public Mapping
{
public:
    MacroMapping(var params = var());
    ~MacroMapping();

    FloatParameter * sourceParam;

    void onContainerParameterChangedInternal(Parameter* p) override;

    String getTypeString() const override { return "Macro"; }
};

class MIDIMapping :
    public Mapping,
    public MIDIInputDevice::MIDIInputListener
{
public:
    MIDIMapping(var params = var());
    ~MIDIMapping();

    MIDIDeviceParameter* deviceParam;
    IntParameter* channel;

    enum MIDIType { NOTE, CC, PitchWheel };
    EnumParameter * type;
    IntParameter* pitchOrNumber;
    BoolParameter* learn;
    
    MIDIInputDevice* device;

    void setMIDIDevice(MIDIInputDevice* d);
    void onContainerParameterChangedInternal(Parameter* p) override;

    void noteOnReceived(const int& channel, const int& pitch, const int& velocity) override;
    void noteOffReceived(const int& channel, const int& pitch, const int& velocity) override;
    void controlChangeReceived(const int& channel, const int& number, const int& value) override;
    void pitchWheelReceived(const int& channel, const int &value) override;

    String getTypeString() const override { return "MIDI"; }

};