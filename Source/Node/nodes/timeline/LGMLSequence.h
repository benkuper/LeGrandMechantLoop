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
    LGMLSequence(TimelineNode* node = nullptr);
    ~LGMLSequence();

    void clearItem() override;
    TimelineNode* node; //the timeline node that this sequence is attached to

    void itemAdded(SequenceLayer* layer) override;
    void itemsAdded(Array<SequenceLayer*> layers) override;

    void updateAudioInputs();
    void updateAudioOutputs();

	void selectThis(bool addToSelection = false, bool notify = true) override;


    //void itemRemoved(SequenceLayer* layer) override;

	DECLARE_TYPE("LGML Sequence");
};

class LGMLSequenceManager :
    public SequenceManager
{
public:
    
    LGMLSequenceManager(TimelineNode* node) : node(node) {}
	~LGMLSequenceManager() {}

    TimelineNode* node;
    Sequence* createItem() override { return new LGMLSequence(node); }
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