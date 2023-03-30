/*
  ==============================================================================

	Mapping.cpp
	Created: 3 May 2021 10:23:49am
	Author:  bkupe

  ==============================================================================
*/

#include "Mapping.h"
#include "Macro/MacroManager.h"
#include "Engine/LGMLSettings.h"

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
	//outsideBehaviour = addEnumParameter("Outside Behaviour", "How to behave when values are outside the output range");
	//outsideBehaviour->addOption("Clip", CLIP);//->addOption("Loop", LOOP)/;

	boolBehaviour = addEnumParameter("Boolean Behaviour", "How to behave when setting a boolean. \"Non-zero\" means any positive, non-zero value will be treated as TRUE.\
 \n\"One\" means that only values from 1 and up will be treated as TRUE", false);
	boolBehaviour->addOption("Non-Zero", NONZERO)->addOption("One", ONE)->addOption("Default", DEFAULT);
	boolToggle = addBoolParameter("Toggle Mode", "If checked, boolean values will be used with a toggle", false, false);

	autoFeedback = addBoolParameter("Auto Feedback", "If checked, this will send back the value to the MIDI interface", true);
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
		if (dest->type != Controllable::TRIGGER)
		{
			((Parameter*)dest.get())->removeParameterListener(this);
		}
	}

	dest = c;

	if (dest != nullptr)
	{
		boolBehaviour->setEnabled(dest->type == Parameter::BOOL);
		boolToggle->setEnabled(dest->type == Parameter::BOOL);

		if (dest->type != Controllable::TRIGGER)
		{
			((Parameter*)dest.get())->addParameterListener(this);
		}
	}
}

void Mapping::onContainerParameterChangedInternal(Parameter* p)
{
	if (p == destParam)
	{
		setDest(destParam->target.get());
	}
}

void Mapping::onExternalParameterValueChanged(Parameter* p)
{
	BaseItem::onExternalParameterValueChanged(p);
	if (p == dest && autoFeedback->boolValue())
	{
		isSendingFeedback = true;
		sendFeedback();
		isSendingFeedback = false;
	}
}


void Mapping::process(var value)
{
	if (dest == nullptr || dest.wasObjectDeleted()) return;
	if (isSendingFeedback) return;

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
	//controllables.move(controllables.indexOf(outsideBehaviour), controllables.size() - 1);
	controllables.move(controllables.indexOf(boolBehaviour), controllables.size() - 1);
	controllables.move(controllables.indexOf(boolToggle), controllables.size() - 1);
	controllables.move(controllables.indexOf(autoFeedback), controllables.size() - 1);
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
	midiInterface(nullptr)
{
	bool defaultLearnVal = false;
	if (!Engine::mainEngine->isLoadingFile) defaultLearnVal = LGMLSettings::getInstance()->autoLearnOnCreateMapping->boolValue();

	learn = addBoolParameter("Learn MIDI", "When enabled, this will automatically assign info from the next midi message received", defaultLearnVal);
	learn->isSavable = false;

	interfaceParam = addTargetParameter("Interface", "Interface to receive from", InterfaceManager::getInstance());
	interfaceParam->targetType = TargetParameter::CONTAINER;
	interfaceParam->maxDefaultSearchLevel = 0;
	interfaceParam->typesFilter.add(MIDIInterface::getTypeStringStatic());

	type = addEnumParameter("Type", "Type of Midi message");
	type->addOption("Control Change", CC)->addOption("Note", NOTE)->addOption("PitchWheel", PitchWheel);
	channel = addIntParameter("Channel", "Channel to use for this mapping. 0 means all channels", 1, 1, 16);
	pitchOrNumber = addIntParameter("Number", "The pitch if it's a note, or number if it's a control change", 0, 0, 127);

	controllables.move(controllables.indexOf(destParam), controllables.size() - 1);
	controllables.move(controllables.indexOf(inputRange), controllables.size() - 1);
	controllables.move(controllables.indexOf(outputRange), controllables.size() - 1);
	//controllables.move(controllables.indexOf(outsideBehaviour), controllables.size() - 1);
	controllables.move(controllables.indexOf(boolBehaviour), controllables.size() - 1);
	controllables.move(controllables.indexOf(boolToggle), controllables.size() - 1);
	controllables.move(controllables.indexOf(autoFeedback), controllables.size() - 1);
	inputRange->setPoint(0, 127);

}

MIDIMapping::~MIDIMapping()
{
}

void MIDIMapping::clearItem()
{
	Mapping::clearItem();
	InterfaceManager::getInstance()->removeInterfaceManagerListener(this);
	setMIDIInterface(nullptr);

}

void MIDIMapping::setMIDIInterface(MIDIInterface* d)
{
	if (midiInterface != nullptr)
	{
		midiInterface->removeMIDIInterfaceListener(this);
	}

	midiInterface = d;

	if (midiInterface != nullptr)
	{
		midiInterface->addMIDIInterfaceListener(this);
	}
}

void MIDIMapping::onContainerParameterChangedInternal(Parameter* p)
{
	Mapping::onContainerParameterChangedInternal(p);
	if (p == interfaceParam) setMIDIInterface((MIDIInterface*)interfaceParam->targetContainer.get());
	else if (p == type)
	{
		if (!isCurrentlyLoadingData)
		{
			inputRange->setPoint(0, type->getValueDataAsEnum<MIDIType>() == PitchWheel ? 65535 : 127);
		}
	}
	else if (p == learn)
	{
		if (learn->boolValue()) InterfaceManager::getInstance()->addInterfaceManagerListener(this);
		else InterfaceManager::getInstance()->removeInterfaceManagerListener(this);
	}
}

void MIDIMapping::noteOnReceived(MIDIInterface*, const int& _channel, const int& pitch, const int& velocity)
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

void MIDIMapping::sendFeedback()
{
	if (midiInterface == nullptr) return;
	if (dest->type == Controllable::TRIGGER) return;

	MIDIType mt = type->getValueDataAsEnum<MIDIType>();

	float val = ((Parameter*)dest.get())->floatValue();
	float destVal = jmap(val, outputRange->x, outputRange->y, inputRange->x, inputRange->y);
	float minVal = jmin(inputRange->x, inputRange->y);
	float maxVal = jmax(inputRange->x, inputRange->y);
	destVal = jlimit(minVal, maxVal, destVal);

	if (mt == NOTE)
	{
		midiInterface->sendNoteOn(channel->intValue(), pitchOrNumber->intValue(), destVal);
	}
	else if (mt == CC)
	{
		midiInterface->sendControlChange(channel->intValue(), pitchOrNumber->intValue(), destVal);
	}
	else if (mt == PitchWheel)
	{
		midiInterface->sendPitchWheel(channel->intValue(), destVal);
	}
}

void MIDIMapping::noteOffReceived(MIDIInterface*, const int& _channel, const int& pitch, const int& velocity)
{
	if (type->getValueDataAsEnum<MIDIType>() != NOTE) return;
	if (channel->intValue() > 0 && _channel != channel->intValue()) return;
	if (pitch != pitchOrNumber->intValue()) return;
	process(velocity);
}

void MIDIMapping::controlChangeReceived(MIDIInterface*, const int& _channel, const int& number, const int& velocity)
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

void MIDIMapping::pitchWheelReceived(MIDIInterface*, const int& _channel, const int& value)
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


void MIDIMapping::managerMidiMessageReceived(MIDIInterface* i, const MidiMessage& m)
{
	if (learn->boolValue()) interfaceParam->setValueFromTarget(i);
}

MacroMapping::MacroMapping(var params) :
	Mapping(params)
{
	sourceParam = addFloatParameter("Value", "Value to use as input for this macro", 0, 0, 1);
	controllables.move(controllables.indexOf(destParam), controllables.size() - 1);
	controllables.move(controllables.indexOf(inputRange), controllables.size() - 1);
	controllables.move(controllables.indexOf(outputRange), controllables.size() - 1);
	//controllables.move(controllables.indexOf(outsideBehaviour), controllables.size() - 1);	
	controllables.move(controllables.indexOf(boolBehaviour), controllables.size() - 1);
	controllables.move(controllables.indexOf(boolToggle), controllables.size() - 1);
	controllables.move(controllables.indexOf(autoFeedback), controllables.size() - 1);
	inputRange->setPoint(0, 1);
}

MacroMapping::~MacroMapping()
{
}

void MacroMapping::onContainerParameterChangedInternal(Parameter* p)
{
	Mapping::onContainerParameterChangedInternal(p);
	if (p == sourceParam) process(p->getValue());
}
