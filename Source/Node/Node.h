/*
  ==============================================================================

    Node.h
    Created: 15 Nov 2020 8:40:03am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"

class NodeConnection;
class NodeAudioConnection;
class NodeMIDIConnection;
class BaseNodeViewUI;

class NodeAudioProcessor;

class Node :
    public BaseItem
{
public:
    Node(StringRef name = "Node",
        var params = var(),
        bool hasInput = true, bool hasOutput = true,
        bool userCanSetIO = false,
        bool useOutControl = true);

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

    MidiBuffer inMidiBuffer;

    BoolParameter* isNodePlaying;
    IntParameter* numAudioInputs; //if userCanSetIO
    IntParameter* numAudioOutputs; //if userCanSetIO
    FloatParameter* outRMS;
    FloatParameter* outGain;

    float prevGain; //for smooth outGain

    virtual void init(AudioProcessorGraph* graph);
    virtual void initInternal() {}

    void onContainerParameterChangedInternal(Parameter* p) override;

    void setAudioInputs(const int& numInputs, bool updateConfig = true); //auto naming
    void setAudioInputs(const StringArray& inputNames, bool updateConfig = true);
    void setAudioOutputs(const int& numOutputs, bool updateConfig = true); //auto naming
    void setAudioOutputs(const StringArray& outputNames, bool updateConfig = true);
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

    void addInConnection(NodeConnection* c);
    void removeInConnection(NodeConnection* c);
    void addOutConnection(NodeConnection* c);
    void removeOutConnection(NodeConnection* c);

    void setMIDIIO(bool hasInput, bool hasOutput);

    //Audio and IO handling
    virtual void updatePlayConfig(bool notify = true);
    virtual void updatePlayConfigInternal();

    //MIDI
    void receiveMIDIFromInput(Node* n, MidiBuffer& inputBuffer);


    //AudioProcessor calls
    virtual void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) {}
    virtual void releaseResources() {}
    virtual void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages);
    virtual void processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) {}
    virtual void processBlockBypassed(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) {}

 
    class NodeListener
    {
    public:
        virtual ~NodeListener() {}
        virtual void audioInputsChanged(Node *n) {}
        virtual void audioOutputsChanged(Node* n) {}
        virtual void midiInputChanged(Node* n) {}
        virtual void midiOutputChanged(Node * n) {}
        virtual void nodePlayConfigUpdated(Node* n) {};
    };

    ListenerList<NodeListener> nodeListeners;
    void addNodeListener(NodeListener* newListener) { nodeListeners.add(newListener); }
    void removeNodeListener(NodeListener* listener) { nodeListeners.remove(listener); }

    DECLARE_ASYNC_EVENT(Node, Node, node, ENUM_LIST(INPUTS_CHANGED, OUTPUTS_CHANGED, MIDI_INPUT_CHANGED, MIDI_OUTPUT_CHANGED))

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
    virtual bool acceptsMidi() const override { return false; }
    virtual bool producesMidi() const override { return false; }

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