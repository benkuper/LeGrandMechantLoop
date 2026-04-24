/*
  ==============================================================================

    AudioManagerEditor.cpp
    Created: 15 Nov 2020 5:47:33pm
    Author:  bkupe

  ==============================================================================
*/

#include "AudioManagerEditor.h"

AudioManagerEditor::AudioManagerEditor(AudioManager* manager, bool isRoot) :
    GenericControllableContainerEditor(manager, isRoot),
    manager(manager),
    selector(manager->am, 0, 32, 0, 32, false, false, false, false)
{
    selector.setSize(100, 300);
    selector.setVisible(!container->editorIsCollapsed);
    selector.addMouseListener(this, true);
    addAndMakeVisible(selector);
    if (selector.isVisible()) setSize(100, selector.getBottom() + headerHeight + headerGap);
}

AudioManagerEditor::~AudioManagerEditor()
{
    selector.removeMouseListener(this);
}

void AudioManagerEditor::setCollapsed(bool value, bool force, bool animate, bool doNotRebuild)
{
    GenericControllableContainerEditor::setCollapsed(value, force, animate, doNotRebuild);
    selector.setVisible(!container->editorIsCollapsed);
}

void AudioManagerEditor::resizedInternalContent(Rectangle<int>& r)
{
    selector.setBounds(r.withHeight(selector.getHeight()));
    r.setY(selector.getHeight());
    r.setHeight(0);
}

void AudioManagerEditor::mouseDown(const MouseEvent& e)
{
    ignoreUnused(e);

    if (manager != nullptr) manager->markNextAudioChangeAsManualFromUI();
}
