/*
  ==============================================================================

	Mapping.cpp
	Created: 3 May 2021 10:23:49am
	Author:  bkupe

  ==============================================================================
*/

#include "Mapping.h"
#include "Macro/MacroManager.h"

Mapping::Mapping(var params) :
	BaseItem("Mapping"),
	prevVal(false)
{
	destParam = addTargetParameter("Target", "Target parameter to change");
	destParam->typesFilter.add(FloatParameter::getTypeStringStatic());
	destParam->typesFilter.add(IntParameter::getTypeStringStatic());
	destParam->typesFilter.add(BoolParameter::getTypeStringStatic());
	destParam->typesFilter.add(Trigger::getTypeStringStatic());

	inputRange = addPoint2DParameter("Input Range", "Working range to process the input with");
	inputRange->setPoint(0, 1);
	outputRange = addPoint2DParameter("Output Range", "Working range to process the output with");
	outputRange->setPoint(0, 1);
	outsideBehaviour = addEnumParameter("Outside Behaviour", "How to behave when values are outside the output range");
	outsideBehaviour->addOption("Clip", CLIP);//->addOption("Loop", LOOP)/;

	boolBehaviour = addEnumParameter("Boolean Behaviour", "How to behave when setting a boolean. \"Non-zero\" means any positive, non-zero value will be treated as TRUE.\
 \n\"One\" means that only values from 1 and up will be treated as TRUE", false);
	boolBehaviour->addOption("Non-Zero", NONZERO)->addOption("One", ONE)->addOption("Default", DEFAULT);
	boolToggle = addBoolParameter("Toggle Mode", "If checked, boolean values will be used with a toggle", false, false);
}

Mapping::~Mapping()
{
	setDest(nullptr);
}

void Mapping::setDest(Controllable* c)
{
	if (dest != nullptr)
	{
		//
	}

	dest = c;

	if (dest != nullptr)
	{
		boolBehaviour->setEnabled(dest->type == Parameter::BOOL);
		boolToggle->setEnabled(dest->type == Parameter::BOOL);
	}
}

void Mapping::onContainerParameterChangedInternal(Parameter* p)
{
	if (p == destParam)
	{
		setDest(destParam->target.get());
	}
}

void Mapping::process(var value)
{
	if (dest == nullptr || dest.wasObjectDeleted()) return;

	float val = value.isInt() ? (float)(int)value : (float)value;
	float destVal = jmap(val, inputRange->x, inputRange->y, outputRange->x, outputRange->y);
	float minVal = jmin(outputRange->x, outputRange->y);
	float maxVal = jmax(outputRange->x, outputRange->y);
	destVal = jlimit(minVal, maxVal, destVal);

	if (dest->type == Controllable::TRIGGER)
	{
		if (destVal > 0) ((Trigger*)dest.get())->trigger();
	}
	else
	{
		Parameter* p = (Parameter*)dest.get();
		if (p->type == Parameter::BOOL)
		{
			BoolBehaviour b = boolBehaviour->getValueDataAsEnum<BoolBehaviour>();

			bool bVal = false;
			switch (b)
			{
			case DEFAULT: bVal = destVal; break;
			case NONZERO: bVal = destVal > 0; break;
			case ONE: bVal = destVal >= 1; break;
			}

			if (boolToggle->boolValue())
			{
				if (bVal && prevVal != bVal) p->setValue(!p->boolValue());
			}
			else p->setValue(bVal);

			prevVal = bVal;
		}
		else
		{
			p->setValue(destVal);
		}
	}
}


GenericMapping::GenericMapping(var params) :
	Mapping(params)
{
	sourceParam = addTargetParameter("Source", "The source parameter to get the value from", nullptr);
	controllables.move(controllables.indexOf(destParam), controllables.size() - 1);
	controllables.move(controllables.indexOf(inputRange), controllables.size() - 1);
	controllables.move(controllables.indexOf(outputRange), controllables.size() - 1);
	controllables.move(controllables.indexOf(outsideBehaviour), controllables.size() - 1);
	controllables.move(controllables.indexOf(boolBehaviour), controllables.size() - 1);
	controllables.move(controllables.indexOf(boolToggle), controllables.size() - 1);
}

GenericMapping::~GenericMapping()
{
}

void GenericMapping::setSource(Controllable* c)
{

	if (source != nullptr)
	{
		if (source->type == Controllable::TRIGGER) ((Trigger*)source.get())->removeTriggerListener(this);
		else ((Parameter*)source.get())->removeParameterListener(this);
	}

	source = c;

	if (source != nullptr)
	{
		if (source->type == Controllable::TRIGGER) ((Trigger*)source.get())->addTriggerListener(this);
		else ((Parameter*)source.get())->addParameterListener(this);
	}
}

void GenericMapping::onContainerParameterChangedInternal(Parameter* p)
{
	Mapping::onContainerParameterChangedInternal(p);
	if (p == sourceParam) setSource((Parameter*)sourceParam->target.get());
}

void GenericMapping::onExternalParameterValueChanged(Parameter* p)
{
	Mapping::onExternalParameterValueChanged(p);
	if (p == source.get()) process(p->getValue());
}

void GenericMapping::onExternalTriggerTriggered(Trigger* t)
{
	Mapping::onExternalTriggerTriggered(t);
	process(1);
	process(0);
}

MIDIMapping::MIDIMapping(var params) :
	Mapping(params),
	device(nullptr)
{
	deviceParam = new MIDIDeviceParameter("Device");
	addParameter(deviceParam);
	type = addEnumParameter("Type", "Type of Midi message");
	type->addOption("Control Change", CC)->addOption("Note", NOTE)->addOption("PitchWheel", PitchWheel);
	channel = addIntParameter("Channel", "Channel to use for this mapping. 0 means all channels", 0, 0, 16);
	pitchOrNumber = addIntParameter("Number", "The pitch if it's a note, or number if it's a control change", 0, 0, 127);

	learn = addBoolParameter("Learn MIDI", "When enabled, this will automatically assign info from the next midi message received", false);
	learn->isSavable = false;

	controllables.move(controllables.indexOf(destParam), controllables.size() - 1);
	controllables.move(controllables.indexOf(inputRange), controllables.size() - 1);
	controllables.move(controllables.indexOf(outputRange), controllables.size() - 1);
	controllables.move(controllables.indexOf(outsideBehaviour), controllables.size() - 1);
	controllables.move(controllables.indexOf(boolBehaviour), controllables.size() - 1);
	controllables.move(controllables.indexOf(boolToggle), controllables.size() - 1);
	inputRange->setPoint(0, 127);
}

MIDIMapping::~MIDIMapping()
{
	setMIDIDevice(nullptr);
}

void MIDIMapping::setMIDIDevice(MIDIInputDevice* d)
{
	if (device != nullptr)
	{
		device->removeMIDIInputListener(this);
	}

	device = d;

	if (device != nullptr)
	{
		device->addMIDIInputListener(this);
	}
}

void MIDIMapping::onContainerParameterChangedInternal(Parameter* p)
{
	Mapping::onContainerParameterChangedInternal(p);
	if (p == deviceParam) setMIDIDevice(deviceParam->inputDevice);
	else if (p == type)
	{
		if (!isCurrentlyLoadingData)
		{
			inputRange->setPoint(0, type->getValueDataAsEnum<MIDIType>() == PitchWheel ? 65535 : 127);
		}
	}
}

void MIDIMapping::noteOnReceived(const int& _channel, const int& pitch, const int& velocity)
{
	if (learn->boolValue())
	{
		type->setValueWithData(NOTE);
		channel->setValue(_channel);
		pitchOrNumber->setValue(pitch);
		learn->setValue(false);
	}
	
	if (type->getValueDataAsEnum<MIDIType>() != NOTE) return;
	if (channel->intValue() > 0 && _channel != channel->intValue()) return;
	if (pitch != pitchOrNumber->intValue()) return;
	
	process(velocity);
}

void MIDIMapping::noteOffReceived(const int& _channel, const int& pitch, const int& velocity)
{
	if (type->getValueDataAsEnum<MIDIType>() != NOTE) return;
	if (channel->intValue() > 0 && _channel != channel->intValue()) return;
	if (pitch != pitchOrNumber->intValue()) return;
	process(velocity);
}

void MIDIMapping::controlChangeReceived(const int& _channel, const int& number, const int& velocity)
{
	if (learn->boolValue())
	{
		type->setValueWithData(CC);
		channel->setValue(_channel);
		pitchOrNumber->setValue(number);
		learn->setValue(false);
	}
	
	if (type->getValueDataAsEnum<MIDIType>() != CC) return;
	if (channel->intValue() > 0 && _channel != channel->intValue()) return;
	if (number != pitchOrNumber->intValue()) return;
	process(velocity);
}

void MIDIMapping::pitchWheelReceived(const int& _channel, const int& value)
{
	if (learn->boolValue())
	{
		type->setValueWithData(PitchWheel);
		channel->setValue(_channel);
		pitchOrNumber->setValue(0);
		learn->setValue(false);
	}
	
	if (type->getValueDataAsEnum<MIDIType>() != PitchWheel) return;
	if (channel->intValue() > 0 && _channel != channel->intValue()) return;
	process(value);
}

MacroMapping::MacroMapping(var params) :
	Mapping(params)
{
	sourceParam = addFloatParameter("Value", "Value to use as input for this macro", 0, 0, 1);
	controllables.move(controllables.indexOf(destParam), controllables.size() - 1);
	controllables.move(controllables.indexOf(inputRange), controllables.size() - 1);
	controllables.move(controllables.indexOf(outputRange), controllables.size() - 1);
	controllables.move(controllables.indexOf(outsideBehaviour), controllables.size() - 1);	
	controllables.move(controllables.indexOf(boolBehaviour), controllables.size() - 1);
	controllables.move(controllables.indexOf(boolToggle), controllables.size() - 1); 
	inputRange->setPoint(0, 1);
}

MacroMapping::~MacroMapping()
{
}

void MacroMapping::onContainerParameterChangedInternal(Parameter* p)
{
	Mapping::onContainerParameterChangedInternal(p);
	if(p == sourceParam) process(p->getValue());
}
