/*
  ==============================================================================

    EnablingNetworkControllableContainerEditor.h
    Created: 19 Jan 2018 4:48:00pm
    Author:  Ben

  ==============================================================================
*/

#pragma once

class EnablingNetworkControllableContainerEditor :
	public EnablingControllableContainerEditor
{
public:
	EnablingNetworkControllableContainerEditor(EnablingControllableContainer * cc, bool isRoot);
	~EnablingNetworkControllableContainerEditor();

	void resizedInternalHeader(Rectangle<int> &r) override;

	Label ipLabel;

	static InspectableEditor * create( bool isRoot, Array<ControllableContainer*> cc) { return new EnablingNetworkControllableContainerEditor((EnablingControllableContainer*)cc[0], isRoot); }
};