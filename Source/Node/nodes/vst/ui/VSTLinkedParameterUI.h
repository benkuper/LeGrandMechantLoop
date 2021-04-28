/*
  ==============================================================================

    VSTLinkedParameterUI.h
    Created: 7 Dec 2020 7:01:39pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once

class VSTParameterContainerEditor :
    public GenericControllableContainerEditor
{
public:
    VSTParameterContainerEditor(VSTParameterContainer* vstParamContainer, bool isRoot);
    ~VSTParameterContainerEditor();

    VSTParameterContainer* vstParamContainer;
    std::unique_ptr<ImageButton> addParamBT;
    void resizedInternalHeader(Rectangle<int>& r) override;

    
    void buttonClicked(Button* b) override;

    void fillPopupMenuForGroup(const AudioProcessorParameterGroup  *group, PopupMenu & menu);
};

class VSTLinkedParameterEditor :
    public ParameterEditor,
    public VSTParameterLink::AsyncListener
{
public:
    VSTLinkedParameterEditor(VSTParameterLink * pLink, bool isRoot);
    ~VSTLinkedParameterEditor();

    VSTParameterLink* pLink;

    std::unique_ptr<ImageButton> linkBT;

    void paint(Graphics& g) override;
    void paintOverChildren(Graphics& g) override;

    void resizedInternal(Rectangle<int> &r) override;

    void buttonClicked(Button* b) override;

    void newMessage(const VSTParameterLink::VSTParameterLinkEvent& e) override;
};