/*
  ==============================================================================

	SamplerNode.h
	Created: 14 Apr 2021 8:40:43am
	Author:  bkupe

  ==============================================================================
*/

#pragma once

class SamplerNode :
	public Node,
	public MidiKeyboardStateListener
{
public:
	SamplerNode(var params);
	virtual ~SamplerNode();

	enum PlayMode { HIT_LOOP, HIT_ONESHOT, PEEK, KEEP };
	enum HitMode { PIANO, HIT_RESET, HIT_FULL, TOGGLE };


	ControllableContainer playCC;
	IntParameter* numChannels;
	BoolParameter* monitor;
	EnumParameter* playMode;
	EnumParameter* hitMode;
	Trigger* computeAutoKeysTrigger;
	IntParameter* startAutoKey;
	IntParameter* endAutoKey;
	BoolParameter* autoKeyLiveMode;

	ControllableContainer recordCC;
	BoolParameter* isRecording;
	IntParameter* fadeTimeMS;

	BoolParameter* clearMode;
	enum ClearMode { LAST_PLAYED, LAST_RECORDED };
	EnumParameter* clearLastMode;
	Trigger* clearLastRecordedTrigger;
	Trigger* clearAllNotesTrigger;

	ControllableContainer adsrCC;
	FloatParameter* attack;
	FloatParameter* attackCurve;
	FloatParameter* decay;
	DecibelFloatParameter* sustain;
	FloatParameter* release;
	FloatParameter* releaseCurve;
	ControllableContainer noteStatesCC;

	//Bank
	ControllableContainer libraryCC;
	FileParameter* libraryFolder;
	EnumParameter* currentLibrary;
	IntParameter* currentBank;
	StringParameter* bankDescription;
	Trigger* saveBankTrigger;
	Trigger* showFolderTrigger;

	//ui
	BoolParameter* showKeyboard;


	int recordingNote;
	int lastRecordedNote;
	int lastPlayedNote;

	int viewStartKey;

	SpinLock recLock;

	bool isUpdatingLibrary;
	File bankFolder;

	std::unique_ptr<RingBuffer<float>> ringBuffer;
	AudioSampleBuffer preRecBuffer;
	int recordedSamples;

	MidiKeyboardState keyboardState;


	enum NoteState { EMPTY, RECORDING, FILLED, PROCESSING, PLAYING };
	class SamplerNote :
		public Thread
	{
	public:
		SamplerNote();
		~SamplerNote();
		EnumParameter* state;
		int playingSample = 0;
		int jumpGhostSample = -1;
		float velocity = 0;
		CurvedADSR adsr;
		bool oneShotted = false;

		AudioSampleBuffer buffer;

		//offline pitching
		double shifting = 0;
		int fadeNumSamples = 0;

		//rt pitch shifting // now trying to do it offline
		AudioSampleBuffer rtPitchedBuffer;
		SamplerNote* autoKeyFromNote = nullptr;
		int rtPitchReadSample = 0;

		SpinLock pitcherLock;
		std::unique_ptr<RubberBand::RubberBandStretcher> pitcher;

		void setAutoKey(SamplerNote* remoteNote, double shift = 0);
		void computeAutoKey(SamplerNote* remoteNote, double shift = 0, int fadeSamples = 0);

		void run();

		void reset();

		bool hasContent() const { return buffer.getNumSamples() > 0; }
		bool isProxyNote() const { return autoKeyFromNote != nullptr; }
	};

	OwnedArray<SamplerNote> samplerNotes;

	void clearNote(int note);
	void clearAllNotes();
	void resetAllNotes();

	void computeAutoKeys();


	void updateBuffers();
	void updateRingBuffer();

	void startRecording(int note);
	void stopRecording();

	void onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c) override;
	void controllableStateChanged(Controllable* c) override;

	void midiMessageReceived(MIDIInterface* i, const MidiMessage& m) override;

	virtual void handleNoteOn(MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity) override;
	virtual void handleNoteOff(MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity) override;

	virtual int getFadeNumSamples(); //for ring buffer fade

	void updateLibraries(bool loadAfter = true);
	void setBankIndex(int index, bool force = false);

	void saveBankSamples();
	void loadBankSamples();


	var getJSONData() override;
	void loadJSONDataItemInternal(var data) override;

	void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
	void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;
	//void processBlockBypassed(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;


	String getTypeString() const override { return getTypeStringStatic(); }
	static const String getTypeStringStatic() { return "Sampler"; }

	BaseNodeViewUI* createViewUI() override;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SamplerNode)
};