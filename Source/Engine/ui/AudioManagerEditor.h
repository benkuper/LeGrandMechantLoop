/*
  ==============================================================================

    AudioManagerEditor.h
    Created: 15 Nov 2020 5:47:33pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once
#include "../AudioManager.h"

class AudioManagerEditor :
	public GenericControllableContainerEditor
{
public:
	AudioManagerEditor(AudioManager * manager, bool isRoot);
	~AudioManagerEditor();

	AudioManager * manager;
	AudioDeviceSelectorComponent selector;

	void setCollapsed(bool value, bool force = false, bool animate = true, bool doNotRebuild = false) override;
	void resizedInternalContent(Rectangle<int>& r) override;
};