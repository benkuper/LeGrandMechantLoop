/*
  ==============================================================================

    SamplerNodeUI.h
    Created: 14 Apr 2021 8:42:07am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

class SamplerMidiKeyboardComponent :
    public MidiKeyboardComponent
{
 public:
     SamplerMidiKeyboardComponent(SamplerNode* n, MidiKeyboardState& state,
            MidiKeyboardComponent::Orientation orientation);

protected :
    SamplerNode* samplerNode;
    virtual void drawWhiteNote(int midiNoteNumber,
        Graphics& g, Rectangle<float> area,
        bool isDown, bool isOver,
        Colour lineColour, Colour textColour) override;

    virtual void drawBlackNote(int midiNoteNumber,
        Graphics& g, Rectangle<float> area,
        bool isDown, bool isOver,
        Colour noteFillColour) override;
};

class SamplerNodeViewUI :
    public NodeViewUI<SamplerNode>,
    public ChangeListener
{
public:
    SamplerNodeViewUI(SamplerNode* n);
    ~SamplerNodeViewUI();

    std::unique_ptr<ImageButton> editHeaderBT;
    std::unique_ptr<MIDIDeviceParameterUI> midiParamUI;

    SamplerMidiKeyboardComponent midiComp;

    void resizedInternalHeader(Rectangle<int>& r) override;
    void resizedInternalContentNode(Rectangle<int>& r) override;
    void controllableFeedbackUpdateInternal(Controllable* c) override;


    // Inherited via ChangeListener
    virtual void changeListenerCallback(ChangeBroadcaster* source) override;

};
