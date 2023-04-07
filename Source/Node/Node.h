/*
  ==============================================================================

	Node.h
	Created: 15 Nov 2020 8:40:03am
	Author:  bkupe

  ==============================================================================
*/

#pragma once

class NodeConnection;
class NodeAudioConnection;
class NodeMIDIConnection;
class BaseNodeViewUI;

class NodeAudioProcessor;


class Node :
	public BaseItem,
	public MIDIInterface::MIDIInterfaceListener
{
public:
	Node(StringRef name = "Node",
		var params = var(),
		bool hasInput = true, bool hasOutput = true,
		bool userCanSetIO = false,
		bool useOutControl = true,
		bool canHaveMidiDeviceIn = false,
		bool canHaveMidiDeviceOut = false);

	virtual ~Node();
	virtual void clearItem() override;

	AudioProcessorGraph* graph;
	NodeAudioProcessor* processor;

	AudioProcessorGraph::NodeID nodeGraphID;
	AudioProcessorGraph::Node::Ptr nodeGraphPtr;

	StringArray audioInputNames;
	StringArray audioOutputNames;

	bool hasAudioInput; //defines if this processor is supposed to get input, even if the number of channels is zero
	bool hasAudioOutput; //defines if this processor is supposed to provide output, even if the number of channels is zero
	bool hasMIDIInput;
	bool hasMIDIOutput;

	Array<NodeAudioConnection*> inAudioConnections;
	Array<NodeAudioConnection*> outAudioConnections;
	Array<NodeMIDIConnection*> inMidiConnections;
	Array<NodeMIDIConnection*> outMidiConnections;
	Array<float> connectionsActivityLevels;


	std::unique_ptr<ControllableContainer> midiCC;
	TargetParameter* midiInterfaceParam;
	BoolParameter* pedalSustain;
	BoolParameter* forceSustain;
	std::unique_ptr<ControllableContainer> channelFilterCC;
	Array<BoolParameter*> midiChannels;
	BoolParameter* logIncomingMidi;
	BoolParameter* forceNoteOffOnEnabled;

	MidiBuffer inMidiBuffer;
	MidiMessageCollector midiCollector;
	HashMap<int, int> sustainedNotes; //keep track of sustain

	BoolParameter* isNodePlaying;
	IntParameter* numAudioInputs; //if userCanSetIO
	IntParameter* numAudioOutputs; //if userCanSetIO
	std::unique_ptr<VolumeControl> outControl;

	ControllableContainer viewCC;
	BoolParameter* showOutControl;

	MIDIInterface* midiInterface;


	const int anticlickBlocks = 10; //number of blocks to do the transition
	int bypassAntiClickCount; //anti-click on processBlock

	virtual void init(AudioProcessorGraph* graph);
	virtual void initInternal() {}

	virtual void onContainerParameterChangedInternal(Parameter* p) override;
	virtual void onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c) override;

	virtual void setAudioInputs(const int& numInputs, bool updateConfig = true); //auto naming
	virtual void setAudioInputs(const StringArray& inputNames, bool updateConfig = true);
	virtual void setAudioOutputs(const int& numOutputs, bool updateConfig = true); //auto naming
	virtual void setAudioOutputs(const StringArray& outputNames, bool updateConfig = true);
	virtual void autoSetNumAudioInputs(); //if userCanSetIO
	virtual void autoSetNumAudioOutputs(); //if userCanSetIO

	//helpers
	int getNumAudioInputs() const { return audioInputNames.size(); }
	int getNumAudioOutputs() const { return audioOutputNames.size(); }

	//handle after audio io has been set
	virtual void updateAudioInputs(bool updateConfig = true);
	virtual void updateAudioInputsInternal() {}
	virtual void updateAudioOutputs(bool updateConfig = true);
	virtual void updateAudioOutputsInternal() {}

	virtual void addInConnection(NodeConnection* c);
	virtual void removeInConnection(NodeConnection* c);
	virtual void addOutConnection(NodeConnection* c);
	virtual void removeOutConnection(NodeConnection* c);

	void setMIDIIO(bool hasInput, bool hasOutput);
	virtual void setMIDIInterface(MIDIInterface* i);
	//virtual void setMIDIOutDevice(MIDIOutputDevice* d);


	virtual void updatePlayConfig(bool notify = true);
	virtual void updatePlayConfigInternal();

	//MIDI
	//virtual void receiveMIDIFromInput(Node* n, MidiBuffer& inputBuffer);
	virtual void midiMessageReceived(MIDIInterface* i, const MidiMessage& m) override;
	virtual void updateSustainedNotes();

	//AudioProcessor calls
	virtual void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock);
	virtual void releaseResources() {}
	virtual void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages);
	virtual void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) {}
	virtual void processBlockBypassed(AudioBuffer<float>& buffer, MidiBuffer& midiMessages);

	virtual void bypassInternal() {}

	virtual var getJSONData() override;
	virtual void loadJSONDataItemInternal(var data) override;

	class NodeListener
	{
	public:
		virtual ~NodeListener() {}
		virtual void audioInputsChanged(Node* n) {}
		virtual void audioOutputsChanged(Node* n) {}
		virtual void midiInputChanged(Node* n) {}
		virtual void midiOutputChanged(Node* n) {}
		virtual void nodePlayConfigUpdated(Node* n) {};
	};

	ListenerList<NodeListener> nodeListeners;
	void addNodeListener(NodeListener* newListener) { nodeListeners.add(newListener); }
	void removeNodeListener(NodeListener* listener) { nodeListeners.remove(listener); }

	DECLARE_ASYNC_EVENT(Node, Node, node, ENUM_LIST(INPUTS_CHANGED, OUTPUTS_CHANGED, MIDI_INPUT_CHANGED, MIDI_OUTPUT_CHANGED, VIEW_FILTER_UPDATED))

		virtual BaseNodeViewUI* createViewUI();

	WeakReference<Node>::Master masterReference;
};

/* AudioProcessor */
class NodeAudioProcessor :
	public AudioProcessor
{
public:
	NodeAudioProcessor(Node* n) : node(n), suspendCount(0) {}
	~NodeAudioProcessor() {}

	Node* node;
	int suspendCount;

	//Helpers
	void suspend();
	void resume();

	class Suspender
	{
	public:
		Suspender(NodeAudioProcessor* proc);
		NodeAudioProcessor* proc;
		~Suspender();
	};

	virtual const String getName() const override { return node->getTypeString(); }
	virtual void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override { node->prepareToPlay(sampleRate, maximumExpectedSamplesPerBlock); }
	virtual void releaseResources() override {}
	virtual void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override { return node->processBlock(buffer, midiMessages); }
	virtual void processBlockBypassed(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override { return node->processBlockBypassed(buffer, midiMessages); }
	virtual double getTailLengthSeconds() const override { return 0; }
	virtual bool acceptsMidi() const override { return node->hasMIDIInput; }
	virtual bool producesMidi() const override { return node->hasMIDIOutput; }

	virtual void numChannelsChanged() override;

	virtual AudioProcessorEditor* createEditor() override { return nullptr; }
	virtual bool hasEditor() const override { return false; }
	virtual int getNumPrograms() override { return 0; }
	virtual int getCurrentProgram() override { return 0; }
	virtual void setCurrentProgram(int index) override {}
	virtual const String getProgramName(int index) override { return "[NoProgram]"; }
	virtual void changeProgramName(int index, const String& newName) override {}
	virtual void getStateInformation(juce::MemoryBlock& destData) override {}
	virtual void setStateInformation(const void* data, int sizeInBytes) override {}
};

typedef NodeAudioProcessor::Suspender ScopedSuspender;

