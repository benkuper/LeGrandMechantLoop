/*
 ==============================================================================

 Copyright © Organic Orchestra, 2017

 This file is part of LGML. LGML is a software to manipulate sound in real-time

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation (version 3 of the License).

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

 ==============================================================================
 */

static Array<MIDIClock*, CriticalSection> allClocks = {};

class MIDIClockRunner : 
	public Thread, 
	public DeletedAtShutdown, 
	public EngineListener 
{
public:
	MIDIClockRunner() :
		Thread("MIDIClockRunner") 
	{
		startThread(Thread::realtimeAudioPriority);
	}

	~MIDIClockRunner() { stopThread(100); }

	void run()override {
		while (!threadShouldExit()) {
			if (allClocks.size() == 0) { // long polling if empty
				wait(1000);
			}
			else {
				int waitTime = (int)(MIDIClock::sendAllClocks());
				waitTime = jmax(waitTime, 1); // avoid too many useless calls by keeping a granularity of 1ms
				wait((int)waitTime);


			}
		}
	}
};

MIDIClockRunner* getClockRunner() {
	static MIDIClockRunner* inst = nullptr;
	if (!inst) {
		inst = new MIDIClockRunner();
	}
	return inst;
}


constexpr int MIDIClockPPQNRes = 24;

MIDIClock::MIDIClock(bool _sendSPP) :
	midiFifo(MIDI_SYNC_QUEUE_SIZE)
{
	Transport::getInstance()->addTransportListener(this);
	delta = 0.0;
	sendSPP = _sendSPP;
}

MIDIClock::~MIDIClock()
{
	stop();
	if (auto tm = Transport::getInstanceWithoutCreating()) {
		tm->removeTransportListener(this);
	}
}

bool MIDIClock::setOutput(MIDIOutputDevice * _midiOut)
{
	midiOut = _midiOut;
	if (!midiOut)
		return false;

	reset();
	return true;
}

bool MIDIClock::start()
{
	allClocks.addIfNotAlreadyThere(this);
	state.ppqn = (int)getPPQWithDelta(MIDIClockPPQNRes);
	_isRunning = true;
	getClockRunner()->notify(); // instanciate or wake if in long polling
	return midiOut != nullptr;

}

void MIDIClock::stop()
{
	_isRunning = false;
	allClocks.removeAllInstancesOf(this);

	if (!midiOut)
		return;
}

bool MIDIClock::isRunning() { return _isRunning.get(); }
#define DEBUG_USELESS_THREAD_CALLS 0
double MIDIClock::sendAllClocks() {
#if DEBUG_USELESS_THREAD_CALLS
	static int numUselessCalls = 0;
	bool sentOneMessage = false;
#endif
	double nextTime = 1000; // long poll if no active
	//auto tm = TimeManager::getInstance();
   // if(!tm->isPlaying() || tm->isSettingTempo->boolValue())return  nextTime;

	for (auto& c : allClocks) {
		if (c->_isRunning.get()) {
			double cTime = c->runClock();
			if (c->state.isPlaying) { nextTime = jmin(nextTime, cTime); };
#if DEBUG_USELESS_THREAD_CALLS
			sentOneMessage |= c->hasSentMessage;
#endif
		}
	}
#if DEBUG_USELESS_THREAD_CALLS
	if (!sentOneMessage) {
		numUselessCalls++;
		if (numUselessCalls % 100 == 0) {
			DBG("uselessCalls :" << numUselessCalls << " :: " << nextTime);
		}
	}
#endif
	return nextTime;
}
double MIDIClock::runClock()
{
	double timeToNext = -1;
	addClockIfNeeded(timeToNext);
	int numR = midiFifo.getNumReady();
	hasSentMessage = false;
	if ((numR > 0) && midiOut) {
		hasSentMessage = true;
		int st1, st2, bs1, bs2, i;
		midiFifo.prepareToRead(numR, st1, bs1, st2, bs2);
		for (i = 0; i < bs1; i++) {
			midiOut->sendMessage(messagesToSend[st1 + i]);
		}
		for (i = 0; i < bs2; i++) { midiOut->sendMessage(messagesToSend[st2 + i]); }
		midiFifo.finishedRead(bs1 + bs2);
	}
	jassert(timeToNext >= 0);

	return timeToNext;
}


double MIDIClock::getPPQWithDelta(int multiplier) {
	auto tm = Transport::getInstance();
	return (tm->getCurrentTimeInBeats() + delta * 1.0 * tm->bpm->doubleValue() / 60000.0) * 1.0 * multiplier;
}

double MIDIClock::ppqToTime(double ppq, int multiplier) {
	auto tm = Transport::getInstance();
	return  ppq / tm->bpm->doubleValue() * 60000.0 / multiplier;
}

void MIDIClock::addClockIfNeeded(double& timeToNextMessage) {

	double newT = (getPPQWithDelta(MIDIClockPPQNRes));
	int curPPQN = state.ppqn.get();
	int maxClockMsg = jmax(0, midiFifo.getFreeSpace() - 4);
	double diff = newT - curPPQN;
	int numClocksToAdd = (int)floor(diff);//+1.0/(2*MIDIClockPPQNRes));//  center error  ?
	if (numClocksToAdd < 0) {
		timeToNextMessage = -numClocksToAdd;
		//DBG(String("waiting clk : ")+ String(numClocksToAdd));
		return;
	}


	if (numClocksToAdd > maxClockMsg) {
		//jassertfalse;
		numClocksToAdd = maxClockMsg;

	}

	appendClocks(numClocksToAdd);
	curPPQN = state.ppqn.get();
	if (newT < curPPQN) { // should wait to catch the clock back
		timeToNextMessage = curPPQN - newT;
	}
	else if (newT < curPPQN + 1) { // should wait nextPPQN
		timeToNextMessage = (curPPQN + 1) - newT;
	}
	else { // we didn't manage to catch the clock in this call
		timeToNextMessage = 0;
	}
	jassert(timeToNextMessage >= 0);
	timeToNextMessage = ppqToTime(timeToNextMessage, MIDIClockPPQNRes);
	jassert(timeToNextMessage >= 0);


}

int MIDIClock::appendCurrentSPP() {
	if (sendSPP) {
		//        jassert(!state.isPlaying);
				//        int64 MIDIBeat = (int64)getPPQWithDelta(4);
		auto tm = Transport::getInstance();
		int MIDIBeat = (int)((tm->getCurrentTimeInBeats()) * 4.0);
		if (MIDIBeat < 0) {
			//            jassertfalse;
		}
		// SPP Msg
		appendOneMsg(MidiMessage::songPositionPointer(MIDIBeat));

		return MIDIBeat;
	}
	return 0;

}


void MIDIClock::reset()
{
	//playStop(false);
	//playStop(true);
	start();
}



void MIDIClock::bpmChanged() {}

void MIDIClock::beatChanged(bool isNewBar)
{
}

void MIDIClock::playStateChanged(bool isPlaying, bool forceRestart)
{
	//only handle time jump
	if (forceRestart && sendSPP) {
		int curPPQN = state.ppqn.get();
		auto tm = Transport::getInstance();
		int nextMIDIBeat = (int)(tm->getCurrentTimeInBeats() * 4.0);
		int curMIDIBeat = (int)(curPPQN * 4.0 / MIDIClockPPQNRes);
		if (curMIDIBeat == nextMIDIBeat) {
			//        if(abs(curMIDIBeat-nextMIDIBeat)<=1 ){
			return; // avoid flooding with little jumps
		}
		//DBG(curMIDIBeat << " to " << nextMIDIBeat);
		bool shouldReset = tm->isCurrentlyPlaying->boolValue(); // pasted on ableton behaviour
		if (shouldReset) { appendOneMsg(MidiMessage::midiStop()); }
		int MIDIBeat = appendCurrentSPP();
		jassert(nextMIDIBeat == MIDIBeat);
		if (shouldReset) {
			bool sendContinue = MIDIBeat > 0;
			appendOneMsg(sendContinue ? MidiMessage::midiContinue() : MidiMessage::midiStart());
		}

		state.ppqn = (int)(MIDIBeat * MIDIClockPPQNRes / 4.0);
		//getClockRunner()->notify(); // avoid notifying in time critical situations
	}

	if (isPlaying) { //is it what we want to check ?
		state.ppqn = (int)getPPQWithDelta(MIDIClockPPQNRes);
		if (state.ppqn.get() < 0) {
			appendClocks(-state.ppqn.get());
		}
		int MIDIBeat = 0;
		if (sendSPP) {
			MIDIBeat = appendCurrentSPP();
		}
		bool sendContinue = MIDIBeat > 0;
		appendOneMsg(sendContinue ? MidiMessage::midiContinue() : MidiMessage::midiStart());
		state.ppqn = (int)(MIDIBeat * MIDIClockPPQNRes / 4.0);
	}
	else {
		appendOneMsg(MidiMessage::midiStop());
	}
	state.isPlaying = isPlaying;
	getClockRunner()->notify();
}


void MIDIClock::appendClocks(const int numClocksToAdd) {
	int st1, st2, bs1, bs2;
	midiFifo.prepareToWrite(numClocksToAdd, st1, bs1, st2, bs2);
	for (int i = st1; i < st1 + bs1; i++) {
		messagesToSend[i] = MidiMessage::midiClock();
	}
	for (int i = st2; i < st2 + bs2; i++) { messagesToSend[i] = MidiMessage::midiClock(); }
	midiFifo.finishedWrite(bs1 + bs2);
	state.ppqn += numClocksToAdd;


}


void MIDIClock::appendOneMsg(const MidiMessage& msg) {
	int st1, st2, bs1, bs2;
	midiFifo.prepareToWrite(1, st1, bs1, st2, bs2);
	if (bs1 > 0) {
		messagesToSend[st1] = msg;
	}
	midiFifo.finishedWrite(1);
}

