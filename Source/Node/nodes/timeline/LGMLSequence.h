/*
  ==============================================================================

    LGMLSequence.h
    Created: 20 Apr 2024 9:05:28pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

class TimelineNode;

class LGMLSequence :
    public Sequence,
    public SequenceLayerManager::ManagerListener
{
public:
    LGMLSequence(TimelineNode* node);
    ~LGMLSequence();

    void clearItem() override;
    TimelineNode* node; //the timeline node that this sequence is attached to

    void itemAdded(SequenceLayer* layer) override;
    void itemsAdded(Array<SequenceLayer*> layers) override;

    void updateAudioInputs();
    void updateAudioOutputs();

    //void itemRemoved(SequenceLayer* layer) override;
};


class LGMLAudioLayer :
    public AudioLayer
{
public:
    LGMLAudioLayer(Sequence* s, var params);
    ~LGMLAudioLayer();

    void updateAudioInputs();
    void updateAudioOutputs();

    int getNodeGraphIDIncrement() override;

    static LGMLAudioLayer* create(Sequence* sequence, var params) { return new LGMLAudioLayer(sequence, params); }
};