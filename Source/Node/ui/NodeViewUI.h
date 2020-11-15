/*
  ==============================================================================

    NodeViewUI.h
    Created: 15 Nov 2020 9:26:57am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "../Node.h"

class NodeViewUI :
    public BaseItemUI<Node>
{
public:
    NodeViewUI(Node* node);
    virtual ~NodeViewUI();
};

template<class T>
class GenericAudioNodeViewUI :
    public NodeViewUI
{
public:
    GenericAudioNodeViewUI(GenericAudioNode<T>* audioNode) : NodeViewUI(audioNode) {}
    ~GenericAudioNodeViewUI() {}

    GenericAudioNode<T> audioNode;
    T* processor;
};