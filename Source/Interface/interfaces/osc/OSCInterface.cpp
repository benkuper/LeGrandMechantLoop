/*
  ==============================================================================

	OSCInterface.cpp
	Created: 29 Oct 2016 7:07:07pm
	Author:  bkupe

  ==============================================================================
*/

OSCInterface::OSCInterface(var params) :
	Interface(getTypeString(), params),
	Thread("OSCZeroconf"),
	localPort(nullptr),
	servus("_osc._udp"),
	receiveCC(nullptr)
{

	receiveCC.reset(new EnablingControllableContainer("OSC Input"));
	receiveCC->customGetEditorFunc = &EnablingNetworkControllableContainerEditor::create;
	addChildControllableContainer(receiveCC.get());

	localPort = receiveCC->addIntParameter("Local Port", "Local Port to bind to receive OSC Messages", 9000, 1024, 65535);
	localPort->warningResolveInspectable = this;

	receiver.registerFormatErrorHandler(&OSCHelpers::logOSCFormatError);
	receiver.addListener(this);

	if (!Engine::mainEngine->isLoadingFile) setupReceiver();

	outputManager.reset(new BaseManager<OSCOutput>("OSC Outputs"));
	outputManager->addBaseManagerListener(this);

	addChildControllableContainer(outputManager.get());

	outputManager->setCanBeDisabled(true);
	if (!Engine::mainEngine->isLoadingFile)
	{
		OSCOutput* o = outputManager->addItem(nullptr, var(), false);
		o->remotePort->setValue(12000);
		if (!Engine::mainEngine->isLoadingFile) setupSenders();
	}

	//Script
	scriptObject.setMethod("send", OSCInterface::sendOSCFromScript);
	scriptObject.setMethod("sendTo", OSCInterface::sendOSCToFromScript);
	scriptObject.setMethod("match", OSCInterface::matchOSCAddrFromScript);
	scriptObject.setMethod("register", OSCInterface::registerOSCCallbackFromScript);

	scriptManager->scriptTemplate += LGMLAssetManager::getScriptTemplate("osc");

	genericSender.connect("0.0.0.0", 1);
}

OSCInterface::~OSCInterface()
{
	receiver.disconnect();

	if (isThreadRunning())
	{
		stopThread(1000);
	}
}

void OSCInterface::setupReceiver()
{
	receiver.disconnect();
	if (receiveCC == nullptr) return;

	if (!receiveCC->enabled->boolValue())
	{
		localPort->clearWarning();
		return;
	}

	//DBG("Local port set to : " << localPort->intValue());
	bool result = receiver.connect(localPort->intValue());

	if (result)
	{
		NLOG(niceName, "Now receiving on port : " + localPort->stringValue());
		if (!isThreadRunning() && !Engine::mainEngine->isLoadingFile) startThread();

		Array<IPAddress> ad;

		IPAddress::findAllAddresses(ad);
		Array<String> ips;
		for (auto& a : ad) ips.add(a.toString());
		ips.sort();
		String s = "Local IPs:";
		for (auto& ip : ips) s += String("\n > ") + ip;

		NLOG(niceName, s);
		localPort->clearWarning();
	}
	else
	{
		NLOGERROR(niceName, "Error binding port " + localPort->stringValue());
		localPort->setWarningMessage("Error binding port " + localPort->stringValue());
	}

}

float OSCInterface::getFloatArg(OSCArgument a)
{
	if (a.isFloat32()) return a.getFloat32();
	if (a.isInt32()) return (float)a.getInt32();
	if (a.isString()) return a.getString().getFloatValue();
	if (a.isColour()) return getIntArg(a);
	return 0;
}

int OSCInterface::getIntArg(OSCArgument a)
{
	if (a.isInt32()) return a.getInt32();
	if (a.isFloat32()) return roundf(a.getFloat32());
	if (a.isString()) return a.getString().getIntValue();
	if (a.isColour()) return a.getColour().toInt32();
	//return c.red << 24 | c.green << 16 | c.blue << 8 | c.alpha;
//}

	return 0;
}

String OSCInterface::getStringArg(OSCArgument a)
{
	if (a.isString()) return a.getString();
	if (a.isInt32()) return String(a.getInt32());
	if (a.isFloat32()) return String(a.getFloat32());
	if (a.isColour()) return OSCHelpers::getColourFromOSC(a.getColour()).toString();
	return "";
}

Colour OSCInterface::getColorArg(OSCArgument a)
{
	if (a.isColour()) return OSCHelpers::getColourFromOSC(a.getColour());
	if (a.isString()) return Colour::fromString(a.getString());
	if (a.isInt32()) return Colour(a.getInt32());
	if (a.isFloat32()) return Colour(a.getFloat32());
	return Colours::black;
}

void OSCInterface::processMessage(const OSCMessage& msg)
{
	if (logIncomingData->boolValue())
	{
		String s = "";
		for (auto& a : msg) s += String(" ") + getStringArg(a);
		NLOG(niceName, msg.getAddressPattern().toString() << " :" << s);
	}

	processMessageInternal(msg);

	if (scriptManager->items.size() > 0)
	{
		Array<var> params;
		params.add(msg.getAddressPattern().toString());
		var args = var(Array<var>()); //initialize force array
		for (auto& a : msg) args.append(OSCInterface::argumentToVar(a));
		params.add(args);
		scriptManager->callFunctionOnAllItems(oscEventId, params);

		for (auto& entry : scriptCallbacks)
			if (std::get<0>(entry).matches(msg.getAddressPattern().toString()))
				scriptManager->callFunctionOnAllItems(std::get<1>(entry), params);
	}

}

void OSCInterface::itemAdded(OSCOutput* output)
{
	output->warningResolveInspectable = this;
}

void OSCInterface::setupSenders()
{
	for (auto& o : outputManager->items) o->setupSender();
}

void OSCInterface::sendOSC(const OSCMessage& msg, String ip, int port)
{
	if (isClearing || outputManager == nullptr) return;
	if (!enabled->boolValue()) return;

	if (!outputManager->enabled->boolValue()) return;

	if (logOutgoingData->boolValue())
	{
		NLOG(niceName, "Send OSC : " << msg.getAddressPattern().toString());
		for (auto& a : msg)
		{
			LOG(getStringArg(a));
		}
	}

	//outActivityTrigger->trigger();

	if (ip.isNotEmpty() && port > 0)
	{
		genericSender.sendToIPAddress(ip, port, msg);
	}
	else
	{
		for (auto& o : outputManager->items) o->sendOSC(msg);
	}
}

void OSCInterface::setupZeroConf()
{
	if (Engine::mainEngine->isClearing || localPort == nullptr) return;

	String nameToAdvertise;
	int portToAdvertise = 0;
	while (nameToAdvertise != niceName || portToAdvertise != localPort->intValue())
	{
		nameToAdvertise = niceName;
		portToAdvertise = localPort->intValue();

		servus.withdraw();
		sleep(400);

		//if (!hasInput) return;

		//DBG("ADVERTISE");
		servus.announce(portToAdvertise, ("Chataigne - " + nameToAdvertise).toStdString());

		if (nameToAdvertise != niceName || localPort->intValue() != portToAdvertise/*|| !hasInput*/)
		{
			//DBG("Name or port changed during advertise, readvertising");
		}
	}

	NLOG(niceName, "Zeroconf service created : " << nameToAdvertise << ":" << portToAdvertise);
}

var OSCInterface::sendOSCFromScript(const var::NativeFunctionArgs& a)
{
	OSCInterface* m = getObjectFromJS<OSCInterface>(a);
	if (!m->enabled->boolValue()) return var();

	if (!checkNumArgs(m->niceName, a, 1)) return var();

	try
	{
		OSCMessage msg(a.arguments[0].toString());

		for (int i = 1; i < a.numArguments; ++i)
		{
			if (a.arguments[i].isArray())
			{
				Array<var>* arr = a.arguments[i].getArray();
				for (auto& aa : *arr) msg.addArgument(varToArgument(aa));
			}
			else
			{
				msg.addArgument(varToArgument(a.arguments[i]));
			}
		}

		m->sendOSC(msg);
	}
	catch (OSCFormatError& e)
	{
		NLOGERROR(m->niceName, "Error sending message : " << e.description);
	}


	return var();
}

var OSCInterface::sendOSCToFromScript(const var::NativeFunctionArgs& a)
{
	OSCInterface* m = getObjectFromJS<OSCInterface>(a);
	if (!m->enabled->boolValue()) return var();
	if (!checkNumArgs(m->niceName, a, 3)) return var();

	try
	{
		OSCMessage msg(a.arguments[2].toString());

		for (int i = 3; i < a.numArguments; ++i)
		{
			if (a.arguments[i].isArray())
			{
				Array<var>* arr = a.arguments[i].getArray();
				for (auto& aa : *arr) msg.addArgument(varToArgument(aa));
			}
			else
			{
				msg.addArgument(varToArgument(a.arguments[i]));
			}
		}

		m->sendOSC(msg, a.arguments[0], a.arguments[1]);
	}
	catch (OSCFormatError& e)
	{
		NLOGERROR(m->niceName, "Error sending message : " << e.description);
	}


	return var();
}

var OSCInterface::matchOSCAddrFromScript(const var::NativeFunctionArgs& a)
{
	OSCInterface* m = getObjectFromJS<OSCInterface>(a);
	if (!checkNumArgs(m->niceName, a, 2)) return var();

	try
	{
		OSCAddress address(a.arguments[0].toString());
		OSCAddressPattern pattern(a.arguments[1].toString());

		return pattern.matches(address);
	}
	catch (OSCFormatError& e)
	{
		NLOGERROR(m->niceName, "match() function called with incorrect parameters: " << e.description);
	}


	return var();
}

var OSCInterface::registerOSCCallbackFromScript(const var::NativeFunctionArgs& a)
{
	OSCInterface* m = getObjectFromJS<OSCInterface>(a);
	if (!checkNumArgs(m->niceName, a, 2)) return var();

	try
	{
		OSCAddressPattern pattern(a.arguments[0].toString());

		if (!Identifier::isValidIdentifier(a.arguments[1].toString()))
		{
			NLOGERROR(m->niceName, "register() function: invalid callback name " << a.arguments[1].toString());
			return var();
		}
		Identifier callbackName(a.arguments[1].toString());

		for (auto& i : m->scriptCallbacks)
			if (pattern == std::get<0>(i) && callbackName == std::get<1>(i))
				return var();

		m->scriptCallbacks.add(std::make_tuple(pattern, callbackName));
	}
	catch (OSCFormatError& e)
	{
		NLOGERROR(m->niceName, "register() function: invalid pattern: " << e.description);
	}

	return var();
}

void OSCInterface::createThruControllable(ControllableContainer* cc)
{
	TargetParameter* p = new TargetParameter("Output module", "Target module to send the raw data to", "", InterfaceManager::getInstance());
	p->targetType = TargetParameter::CONTAINER;
	p->maxDefaultSearchLevel = 0;
	//p->customGetTargetContainerFunc = &ModuleManager::showAndGetModuleOfType<OSCInterface>;
	p->isRemovableByUser = true;
	p->canBeDisabledByUser = true;
	p->saveValueOnly = false;
	cc->addParameter(p);
}


OSCArgument OSCInterface::varToArgument(const var& v)
{
	if (v.isBool()) return OSCArgument(((bool)v) ? 1 : 0);
	else if (v.isInt()) return OSCArgument((int)v);
	else if (v.isInt64()) return OSCArgument((int)v);
	else if (v.isDouble()) return OSCArgument((float)v);
	else if (v.isString()) return OSCArgument(v.toString());
	jassert(false);
	return OSCArgument("error");
}

OSCArgument OSCInterface::varToColorArgument(const var& v)
{
	if (v.isBool()) return OSCArgument(OSCHelpers::getOSCColour((bool)v ? Colours::white : Colours::black));
	else if (v.isInt() || v.isInt64() || v.isDouble())
	{
		int iv = (int)v;
		OSCColour c = OSCColour::fromInt32(iv);// >> 24 & 0xFF | iv >> 16 & 0xFF | iv >> 8 & 0xFF | iv & 0xFF);
		return OSCArgument(c);
	}
	else if (v.isString())
	{
		Colour c = Colour::fromString(v.toString());
		return OSCArgument(OSCHelpers::getOSCColour(c));
	}
	else if (v.isArray() && v.size() >= 3)
	{
		Colour c = Colour::fromFloatRGBA(v[0], v[1], v[2], v.size() >= 4 ? (float)v[3] : 1.0f);
		return OSCArgument(OSCHelpers::getOSCColour(c));
	}

	jassert(false);
	return OSCArgument("error");
}

var OSCInterface::argumentToVar(const OSCArgument& a)
{
	if (a.isFloat32()) return var(a.getFloat32());
	else if (a.isInt32()) return var(a.getInt32());
	else if (a.isString()) return var(a.getString());
	else if (a.isColour())
	{
		OSCColour c = a.getColour();
		return var((int)c.toInt32());
	}
	else if (a.isBlob())
	{
		MemoryBlock blob = a.getBlob();
		var result;
		for (int i = 0; i < blob.getSize(); i++)
		{
			result.append((uint8)blob[i]);
		}

		return result;
	}

	return var("error");
}

var OSCInterface::getJSONData()
{
	var data = Interface::getJSONData();
	
	var inputData = receiveCC->getJSONData();
	if (!inputData.isVoid()) data.getDynamicObject()->setProperty("input", inputData);

	if (outputManager != nullptr)
	{
		var outputsData = outputManager->getJSONData();
		if (!outputsData.isVoid()) data.getDynamicObject()->setProperty("outputs", outputsData);
	}

	return data;
}

void OSCInterface::loadJSONDataInternal(var data)
{
	Interface::loadJSONDataInternal(data);
	receiveCC->loadJSONData(data.getProperty("input", var()));
	if (outputManager != nullptr)
	{
		outputManager->loadJSONData(data.getProperty("outputs", var()));
		setupSenders();
	}

	if (!isThreadRunning()) startThread();

	setupReceiver();
}

void OSCInterface::onContainerParameterChangedInternal(Parameter* p)
{
	Interface::onContainerParameterChangedInternal(p);
	if (p == localPort)
	{
		if (!isCurrentlyLoadingData) setupReceiver();
	}

}

void OSCInterface::onContainerNiceNameChanged()
{
	Interface::onContainerNiceNameChanged();
	if (Engine::mainEngine->isLoadingFile || Engine::mainEngine->isClearing) return;
	if (!isThreadRunning()) startThread();
}

void OSCInterface::onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c)
{
	Interface::onControllableFeedbackUpdateInternal(cc, c);

	if (outputManager != nullptr && c == outputManager->enabled)
	{
		//bool rv = receiveCC != nullptr ? receiveCC->enabled->boolValue() : false;
		bool sv = outputManager->enabled->boolValue();
		for (auto& o : outputManager->items) o->setForceDisabled(!sv);
		//setupIOConfiguration(rv, sv);
	}
	else if (receiveCC != nullptr && c == receiveCC->enabled)
	{
		bool rv = receiveCC->enabled->boolValue();
		//bool sv = outputManager != nullptr ? outputManager->enabled->boolValue() : false;
		//setupIOConfiguration(rv, sv);
		localPort->setEnabled(rv);
		if (!isCurrentlyLoadingData) setupReceiver();

	}
	else if (OSCOutput* o = c->getParentAs<OSCOutput>())
	{
		if (c == o->listenToOutputFeedback)
		{
			if (o->listenToOutputFeedback->boolValue())
			{
				if (o->receiver != nullptr)
				{
					NLOG(niceName, "Feedback enabled, listening also on port " << o->socket->getBoundPort());
					o->receiver->addListener(this);
				}
			}
		}
	}
}

void OSCInterface::oscMessageReceived(const OSCMessage& message)
{
	if (!enabled->boolValue()) return;
	processMessage(message);
}

void OSCInterface::oscBundleReceived(const OSCBundle& bundle)
{
	if (!enabled->boolValue()) return;
	for (auto& m : bundle)
	{
		processMessage(m.getMessage());
	}
}

void OSCInterface::run()
{
	setupZeroConf();
}


///// OSC OUTPUT

OSCOutput::OSCOutput() :
	BaseItem("OSC Output"),
	Thread("OSC output"),
	forceDisabled(false),
	senderIsConnected(false)
{
	isSelectable = false;

	useLocal = addBoolParameter("Local", "Send to Local IP (127.0.0.1). Allow to quickly switch between local and remote IP.", true);
	remoteHost = addStringParameter("Remote Host", "Remote Host to send to.", "127.0.0.1");
	remoteHost->autoTrim = true;
	remoteHost->setEnabled(!useLocal->boolValue());
	remotePort = addIntParameter("Remote port", "Port on which the remote host is listening to", 9000, 1024, 65535);
	listenToOutputFeedback = addBoolParameter("Listen to Feedback", "If checked, this will listen to the (randomly set) bound port of this sender. This is useful when some softwares automatically detect incoming host and port to send back messages.", false);

	if (!Engine::mainEngine->isLoadingFile) setupSender();
}

OSCOutput::~OSCOutput()
{
	stopThread(1000);
}

void OSCOutput::setForceDisabled(bool value)
{
	if (forceDisabled == value) return;
	forceDisabled = value;
	setupSender();
}

void OSCOutput::onContainerParameterChangedInternal(Parameter* p)
{

	if (p == remoteHost || p == remotePort || p == useLocal)
	{
		if (!Engine::mainEngine->isLoadingFile) setupSender();
		if (p == useLocal) remoteHost->setEnabled(!useLocal->boolValue());
	}
	else if (p == enabled)
	{
		if (!Engine::mainEngine->isLoadingFile) setupSender();
	}
	else if (p == listenToOutputFeedback)
	{
		setupSender();
	}
}

//InspectableEditor* OSCOutput::getEditor(bool isRoot)
//{
//	return new OSCOutputEditor(this, isRoot);
//}

void OSCOutput::setupSender()
{
	if (isThreadRunning())
	{
		stopThread(1000);
		// Clear queue
		const ScopedLock sl(queueLock);
		while (!messageQueue.empty())
			messageQueue.pop();
	}

	senderIsConnected = false;
	sender.disconnect();
	socket.reset();

	if (receiver != nullptr) receiver->disconnect();
	receiver.reset();

	if (!enabled->boolValue() || forceDisabled || Engine::mainEngine->isClearing) return;

	String targetHost = useLocal->boolValue() ? "127.0.0.1" : remoteHost->stringValue();
	socket.reset(new DatagramSocket(true));
	socket->setEnablePortReuse(true);
	socket->bindToPort(0);
	senderIsConnected = sender.connectToSocket(*socket, targetHost, remotePort->intValue());

	if (senderIsConnected)
	{
		if (listenToOutputFeedback->boolValue())
		{
			receiver.reset(new OSCReceiver());
			receiver->connectToSocket(*socket);
		}
		startThread();

		NLOG(niceName, "Now sending to " + targetHost + ":" + remotePort->stringValue());
		clearWarning();
	}
	else
	{
		NLOGWARNING(niceName, "Could not connect to " << targetHost << ":" + remotePort->stringValue());
		setWarningMessage("Could not connect to " + targetHost + ":" + remotePort->stringValue());
	}
}

void OSCOutput::sendOSC(const OSCMessage& m)
{
	if (!enabled->boolValue() || forceDisabled || !senderIsConnected) return;

	{
		const ScopedLock sl(queueLock);
		messageQueue.push(std::make_unique<OSCMessage>(m));
	}
	notify();
}

void OSCOutput::run()
{
	while (!Engine::mainEngine->isClearing && !threadShouldExit())
	{
		std::unique_ptr<OSCMessage> msgToSend;

		{
			const ScopedLock sl(queueLock);
			if (!messageQueue.empty())
			{
				msgToSend = std::move(messageQueue.front());
				messageQueue.pop();
			}
		}

		if (msgToSend)
			sender.send(*msgToSend);
		else
			wait(1000); // notify() is called when a message is added to the queue
	}

	// Clear queue
	const ScopedLock sl(queueLock);
	while (!messageQueue.empty())
		messageQueue.pop();
}