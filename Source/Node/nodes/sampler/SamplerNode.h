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
    enum AutoKeyAlgorithm { RESAMPLE, RUBBERBAND };
    
    // --- New RubberBand Enums ---
    enum RBPitchMode { RB_PITCH_HQ, RB_PITCH_CONSISTENT };
    enum RBTransients { RB_TRANS_CRISP, RB_TRANS_MIXED, RB_TRANS_SMOOTH };
    enum RBWindow { RB_WIN_STANDARD, RB_WIN_SHORT, RB_WIN_LONG };


    ControllableContainer playCC;
    IntParameter* numChannels;
    BoolParameter* monitor;
    EnumParameter* playMode;
    EnumParameter* hitMode;
    BoolParameter* autoKeyLiveMode;
    IntParameter* autoKeyFadeTimeMS;
    EnumParameter* autoKeyMode;
    
    // --- New RubberBand Parameters ---
    BoolParameter* rbPreserveFormants;
    EnumParameter* rbPitchMode;
    EnumParameter* rbTransients;
    EnumParameter* rbWindowSize;

    ControllableContainer controlsCC;
    BoolParameter* clearMode;
    enum ClearMode { LAST_PLAYED, LAST_RECORDED };
    EnumParameter* clearLastMode;
    Trigger* clearLastRecordedTrigger;
    Trigger* clearAllNotesTrigger;
    IntParameter* startAutoKey;
    IntParameter* endAutoKey;
    Trigger* computeAutoKeysTrigger;

    ControllableContainer recordCC;
    BoolParameter* isRecording;
    IntParameter* fadeTimeMS;
    FloatParameter* recVolumeThreshold;
    FloatParameter* stopRecVolumeThreshold;

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
    BoolParameter* autoLoadBank;
    Trigger* loadBankTrigger;
    Trigger* saveBankTrigger;
    Trigger* showFolderTrigger;
    Trigger* nextBank;
    Trigger* prevBank;

    //ui
    BoolParameter* showKeyboard;


    int recordingNote;
    int lastRecordedNote;
    int lastPlayedNote;

    int viewStartKey;

    SpinLock recLock;

    bool isUpdatingLibrary;
    File bankFolder;
    int curBankIndex;

    std::unique_ptr<RingBuffer<float>> ringBuffer;
    AudioSampleBuffer preRecBuffer;
    int recordedSamples;

    MidiKeyboardState keyboardState;


    enum NoteState { EMPTY, ONSET, RECORDING, FILLED, PROCESSING, PLAYING };
    class SamplerNote :
        public Thread
    {
    public:
        SamplerNote();
        ~SamplerNote();
        EnumParameter* state;
        int playingSample = 0;
        int jumpGhostSample = -1;
        float prevVelocity = 0;
        float velocity = 0;
        CurvedADSR adsr;
        bool oneShotted = false;

        AudioSampleBuffer buffer;

        //offline pitching
        double shifting = 0;
        int fadeNumSamples = 0;
        int autoKeyAlgorithm = 0;
        
        // --- RubberBand Offline Params ---
        bool optFormants = false;
        int optTransients = 0;
        int optPitchMode = 0;
        int optWindow = 0;

        //rt pitch shifting
        AudioSampleBuffer rtPitchedBuffer;
        SamplerNote* autoKeyFromNote = nullptr;
        int rtPitchReadSample = 0;

        SpinLock pitcherLock;
        std::unique_ptr<RubberBand::RubberBandStretcher> pitcher;

        void setAutoKey(SamplerNote* remoteNote, double shift = 0);
        void computeAutoKey(SamplerNote* remoteNote, double shift, int fadeSamples, int algorithm, bool formants, int transients, int pitchMode, int windowSize);

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

    virtual void handleNoteOn(MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity) override;
    virtual void handleNoteOff(MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity) override;

    virtual int getFadeNumSamples(int fadeMS);

    void updateLibraries(bool loadAfter = true);
    void updateBank();

    void saveBankSamples();
    void loadBankSamples();


    var getJSONData(bool includeNonOverriden = false) override;
    void loadJSONDataItemInternal(var data) override;

    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;


    DECLARE_TYPE("Sampler");

    BaseNodeViewUI* createViewUI() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SamplerNode)
};
