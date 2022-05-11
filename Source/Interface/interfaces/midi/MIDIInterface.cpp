/*
  ==============================================================================

	MIDIInterface.cpp
	Created: 20 Dec 2016 12:35:26pm
	Author:  Ben

  ==============================================================================
*/

MIDIInterface::MIDIInterface(var params) :
	Interface(getTypeString(), params),
	inputDevice(nullptr),
	outputDevice(nullptr)
{

	midiParam = new MIDIDeviceParameter("Devices");
	addParameter(midiParam);

	isConnected = addBoolParameter("Is Connected", "This is checked if the module is connected to at least one input or output device", false);
	isConnected->setControllableFeedbackOnly(true);
	isConnected->isSavable = false;
	//connectionFeedbackRef = isConnected;


	//Script
	scriptObject.setMethod(sendNoteOnId, &MIDIInterface::sendNoteOnFromScript);
	scriptObject.setMethod(sendNoteOffId, &MIDIInterface::sendNoteOffFromScript);
	scriptObject.setMethod(sendCCId, &MIDIInterface::sendCCFromScript);
	scriptObject.setMethod(sendSysexId, &MIDIInterface::sendSysexFromScript);
	scriptObject.setMethod(sendProgramChangeId, &MIDIInterface::sendProgramChangeFromScript);
	scriptObject.setMethod(sendPitchWheelId, &MIDIInterface::sendPitchWheelFromScript);
	scriptObject.setMethod(sendChannelPressureId, &MIDIInterface::sendChannelPressureFromScript);
	scriptObject.setMethod(sendAfterTouchId, &MIDIInterface::sendAfterTouchFromScript);

	scriptManager->scriptTemplate += LGMLAssetManager::getScriptTemplate("midi");
}

MIDIInterface::~MIDIInterface()
{
	if (inputDevice != nullptr) inputDevice->removeMIDIInputListener(this);
	if (outputDevice != nullptr) outputDevice->close();
}


void MIDIInterface::sendNoteOn(int channel, int pitch, int velocity)
{
	if (!enabled->boolValue()) return;
	if (outputDevice == nullptr) return;
	if (logOutgoingData->boolValue()) NLOG(niceName, "Send Note on, channel : " << channel << ", pitch : " << MIDIManager::getNoteName(pitch) << ", velociy " << velocity);
	//outActivityTrigger->trigger();
	outputDevice->sendNoteOn(channel, pitch, velocity);
}

void MIDIInterface::sendNoteOff(int channel, int pitch)
{
	if (!enabled->boolValue()) return;
	if (outputDevice == nullptr) return;
	if (logOutgoingData->boolValue()) NLOG(niceName, "Send Note off, channel : " << channel << ", pitch");
	//outActivityTrigger->trigger();
	outputDevice->sendNoteOff(channel, pitch);
}

void MIDIInterface::sendControlChange(int channel, int number, int value)
{
	if (!enabled->boolValue()) return;
	if (outputDevice == nullptr) return;
	if (logOutgoingData->boolValue()) NLOG(niceName, "Send Control Change, channel : " << channel << ", number : " << number << ", value " << value);
	//outActivityTrigger->trigger();
	outputDevice->sendControlChange(channel, number, value);
}

void MIDIInterface::sendProgramChange(int channel, int program)
{
	if (!enabled->boolValue()) return;
	if (outputDevice == nullptr) return;
	if (logOutgoingData->boolValue()) NLOG(niceName, "Send ProgramChange, channel : " << channel << ", program : " << program);
	//outActivityTrigger->trigger();
	outputDevice->sendProgramChange(channel, program);
}

void MIDIInterface::sendSysex(Array<uint8> data)
{
	if (!enabled->boolValue()) return;
	if (outputDevice == nullptr) return;
	if (logOutgoingData->boolValue())
	{
		String s = "Send Sysex " + String(data.size()) + " bytes : ";
		for (int i = 0; i < data.size(); ++i) s += "\n" + String(data[i]);
		NLOG(niceName, s);
	}
	//outActivityTrigger->trigger();
	outputDevice->sendSysEx(data);
}

void MIDIInterface::sendPitchWheel(int channel, int value)
{
	if (!enabled->boolValue()) return;
	if (outputDevice == nullptr) return;
	if (logOutgoingData->boolValue()) NLOG(niceName, "Send PitchWheel channel : " << channel << ", value : " << value);
	outputDevice->sendPitchWheel(channel, value);
}

void MIDIInterface::sendChannelPressure(int channel, int value)
{
	if (!enabled->boolValue()) return;
	if (outputDevice == nullptr) return;
	if (logOutgoingData->boolValue()) NLOG(niceName, "Send Channel Pressure channel : " << channel << ", value : " << value);
	outputDevice->sendChannelPressure(channel, value);
}

void MIDIInterface::sendAfterTouch(int channel, int note, int value)
{
	if (!enabled->boolValue()) return;
	if (outputDevice == nullptr) return;
	if (logOutgoingData->boolValue()) NLOG(niceName, "Send After touch channel : " << channel << ", note : " << note << ", value : " << value);
	outputDevice->sendAfterTouch(channel, note, value);
}

void MIDIInterface::sendFullFrameTimecode(int hours, int minutes, int seconds, int frames, MidiMessage::SmpteTimecodeType timecodeType)
{
	if (!enabled->boolValue()) return;
	if (outputDevice == nullptr) return;
	if (logOutgoingData->boolValue())
	{
		NLOG(niceName, "Send full frame timecode : " << hours << ":" << minutes << ":" << seconds << "." << frames << " / " << timecodeType);
	}
	outputDevice->sendFullframeTimecode(hours, minutes, seconds, frames, timecodeType);
}

void MIDIInterface::sendMidiMachineControlCommand(MidiMessage::MidiMachineControlCommand command)
{
	if (!enabled->boolValue()) return;
	if (outputDevice == nullptr) return;
	if (logOutgoingData->boolValue())
	{
		NLOG(niceName, "Send midi machine control command : " << command);
	}
	outputDevice->sendMidiMachineControlCommand(command);
}


void MIDIInterface::onContainerParameterChangedInternal(Parameter* p)
{
	Interface::onContainerParameterChangedInternal(p);
	if (p == midiParam)
	{
		updateMIDIDevices();
	}
}

void MIDIInterface::updateMIDIDevices()
{
	MIDIInputDevice* newInput = midiParam->inputDevice;
	//if (inputDevice != newInput)
	//{
	if (inputDevice != nullptr)
	{
		inputDevice->removeMIDIInputListener(this);
	}
	inputDevice = newInput;
	if (inputDevice != nullptr)
	{
		inputDevice->addMIDIInputListener(this);
	}
	//}

	MIDIOutputDevice* newOutput = midiParam->outputDevice;
	//if (outputDevice != newOutput)
	//{
	if (outputDevice != nullptr) outputDevice->close();
	outputDevice = newOutput;
	if (outputDevice != nullptr) outputDevice->open();
	//} 

	isConnected->setValue(inputDevice != nullptr || outputDevice != nullptr);
}

void MIDIInterface::noteOnReceived(const int& channel, const int& pitch, const int& velocity)
{
	if (!enabled->boolValue()) return;
	//inActivityTrigger->trigger();
	if (logIncomingData->boolValue())  NLOG(niceName, "Note On : " << channel << ", " << MIDIManager::getNoteName(pitch) << ", " << velocity);

	midiInterfaceListeners.call(&MIDIInterfaceListener::noteOnReceived, this, channel, pitch, velocity);

	if (scriptManager->items.size() > 0) scriptManager->callFunctionOnAllItems(noteOnEventId, Array<var>(channel, pitch, velocity));
}

void MIDIInterface::noteOffReceived(const int& channel, const int& pitch, const int& velocity)
{
	if (!enabled->boolValue()) return;
	//inActivityTrigger->trigger();
	if (logIncomingData->boolValue()) NLOG(niceName, "Note Off : " << channel << ", " << MIDIManager::getNoteName(pitch) << ", " << velocity);

	midiInterfaceListeners.call(&MIDIInterfaceListener::noteOffReceived, this, channel, pitch, velocity);

	if (scriptManager->items.size() > 0) scriptManager->callFunctionOnAllItems(noteOffEventId, Array<var>(channel, pitch, velocity));
}

void MIDIInterface::controlChangeReceived(const int& channel, const int& number, const int& value)
{
	if (!enabled->boolValue()) return;
	//inActivityTrigger->trigger();
	if (logIncomingData->boolValue()) NLOG(niceName, "Control Change : " << channel << ", " << number << ", " << value);

	midiInterfaceListeners.call(&MIDIInterfaceListener::controlChangeReceived, this, channel, number, value);

	if (scriptManager->items.size() > 0) scriptManager->callFunctionOnAllItems(ccEventId, Array<var>(channel, number, value));
}

void MIDIInterface::sysExReceived(const MidiMessage& msg)
{
	if (!enabled->boolValue()) return;
	//inActivityTrigger->trigger();

	Array<uint8> data(msg.getSysExData(), msg.getSysExDataSize());

	if (logIncomingData->boolValue())
	{
		String log = "Sysex received, " + String(data.size()) + " bytes";
		for (int i = 0; i < data.size(); ++i)
		{
			log += "\n" + String(data[i]);
		}
		NLOG(niceName, log);
	}

	midiInterfaceListeners.call(&MIDIInterfaceListener::sysExReceived, this, msg);
	
	if (scriptManager->items.size() > 0) scriptManager->callFunctionOnAllItems(sysexEventId, Array<var>(data.getRawDataPointer(), data.size()));
}

void MIDIInterface::fullFrameTimecodeReceived(const MidiMessage& msg)
{
	int hours = 0, minutes = 0, seconds = 0, frames = 0;
	MidiMessage::SmpteTimecodeType timecodeType;
	msg.getFullFrameParameters(hours, minutes, seconds, frames, timecodeType);
	if (logIncomingData->boolValue()) NLOG(niceName, "Full frame timecode received : " << hours << ":" << minutes << ":" << seconds << "." << frames << " / " << timecodeType);

	midiInterfaceListeners.call(&MIDIInterfaceListener::fullFrameTimecodeReceived, this, msg);

}

void MIDIInterface::pitchWheelReceived(const int& channel, const int& value)
{
	if (!enabled->boolValue()) return;
	//inActivityTrigger->trigger();
	if (logIncomingData->boolValue()) NLOG(niceName, "Pitch wheel, channel : " << channel << ", value : " << value);

	if (scriptManager->items.size() > 0) scriptManager->callFunctionOnAllItems(pitchWheelEventId, Array<var>(channel, value));

	midiInterfaceListeners.call(&MIDIInterfaceListener::pitchWheelReceived, this, channel, value);
}

void MIDIInterface::channelPressureReceived(const int& channel, const int& value)
{
	if (!enabled->boolValue()) return;
	//inActivityTrigger->trigger();
	if (logIncomingData->boolValue()) NLOG(niceName, "Channel Pressure, channel : " << channel << ", value : " << value);

	if (scriptManager->items.size() > 0) scriptManager->callFunctionOnAllItems(channelPressureId, Array<var>(channel, value));

	midiInterfaceListeners.call(&MIDIInterfaceListener::channelPressureReceived, this, channel, value);
}

void MIDIInterface::afterTouchReceived(const int& channel, const int& note, const int& value)
{
	if (!enabled->boolValue()) return;
	//inActivityTrigger->trigger();
	if (logIncomingData->boolValue()) NLOG(niceName, "After Touch, channel : " << channel << ", note : " << note << ", value : " << value);

	if (scriptManager->items.size() > 0) scriptManager->callFunctionOnAllItems(afterTouchId, Array<var>(channel, note, value));

	midiInterfaceListeners.call(&MIDIInterfaceListener::afterTouchReceived, this, channel, note, value);
}

void MIDIInterface::midiMessageReceived(const MidiMessage& msg)
{
	if (!enabled->boolValue()) return;
	midiInterfaceListeners.call(&MIDIInterfaceListener::midiMessageReceived, this, msg);
}

var MIDIInterface::sendNoteOnFromScript(const var::NativeFunctionArgs& args)
{
	MIDIInterface* m = getObjectFromJS<MIDIInterface>(args);
	if (!m->enabled->boolValue()) return var();
	if (!checkNumArgs(m->niceName, args, 3)) return var();


	m->sendNoteOn(args.arguments[0], args.arguments[1], args.arguments[2]);
	return var();
}

var MIDIInterface::sendNoteOffFromScript(const var::NativeFunctionArgs& args)
{
	MIDIInterface* m = getObjectFromJS<MIDIInterface>(args);
	if (!m->enabled->boolValue()) return var();
	if (!checkNumArgs(m->niceName, args, 2)) return var();


	m->sendNoteOff(args.arguments[0], args.arguments[1]);
	return var();
}

var MIDIInterface::sendCCFromScript(const var::NativeFunctionArgs& args)
{
	MIDIInterface* m = getObjectFromJS<MIDIInterface>(args);
	if (!m->enabled->boolValue()) return var();
	if (!checkNumArgs(m->niceName, args, 3)) return var();

	m->sendControlChange(args.arguments[0], args.arguments[1], args.arguments[2]);
	return var();
}

var MIDIInterface::sendSysexFromScript(const var::NativeFunctionArgs& args)
{
	MIDIInterface* m = getObjectFromJS<MIDIInterface>(args);
	if (!m->enabled->boolValue()) return var();
	if (!checkNumArgs(m->niceName, args, 1)) return var();

	Array<const var*> allArgs;
	for (int i = 0; i < args.numArguments; ++i)
	{
		if (args.arguments[i].isArray())
		{
			for (int j = 0; j < args.arguments[i].size(); j++)
			{
				allArgs.add(&args.arguments[i][j]);
			}
		}
		else
		{
			allArgs.add(&args.arguments[i]);
		}
	}

	int numArgs = allArgs.size();

	Array<uint8> data;
	for (int i = 0; i < numArgs; ++i)
	{
		const var* a = allArgs[i];
		if (a->isInt() || a->isInt64() || a->isDouble() || a->isBool())data.add((uint8)(int)*a);
		else if (a->isString())
		{
			String s = a->toString();
			for (int j = 0; j < s.length(); j++)
			{
				data.add((uint8)s[j]);
			}
		}
	}
	m->sendSysex(data);

	return var();
}

var MIDIInterface::sendProgramChangeFromScript(const var::NativeFunctionArgs& args)
{
	MIDIInterface* m = getObjectFromJS<MIDIInterface>(args);
	if (!m->enabled->boolValue()) return var();
	if (!checkNumArgs(m->niceName, args, 2)) return var();


	m->sendProgramChange(args.arguments[0], args.arguments[1]);
	return var();
}

var MIDIInterface::sendPitchWheelFromScript(const var::NativeFunctionArgs& args)
{
	MIDIInterface* m = getObjectFromJS<MIDIInterface>(args);
	if (!m->enabled->boolValue()) return var();
	if (!checkNumArgs(m->niceName, args, 2)) return var();


	m->sendPitchWheel(args.arguments[0], args.arguments[1]);
	return var();
}

var MIDIInterface::sendChannelPressureFromScript(const var::NativeFunctionArgs& args)
{
	MIDIInterface* m = getObjectFromJS<MIDIInterface>(args);
	if (!m->enabled->boolValue()) return var();
	if (!checkNumArgs(m->niceName, args, 2)) return var();
	m->sendChannelPressure(args.arguments[0], args.arguments[1]);
	return var();
}

var MIDIInterface::sendAfterTouchFromScript(const var::NativeFunctionArgs& args)
{
	MIDIInterface* m = getObjectFromJS<MIDIInterface>(args);
	if (!m->enabled->boolValue()) return var();
	if (!checkNumArgs(m->niceName, args, 3)) return var();

	m->sendAfterTouch(args.arguments[0], args.arguments[1], args.arguments[2]);
	return var();
}


void MIDIInterface::loadJSONDataInternal(var data)
{
	Interface::loadJSONDataInternal(data);
}