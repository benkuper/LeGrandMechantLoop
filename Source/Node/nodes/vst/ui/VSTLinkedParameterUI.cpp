/*
  ==============================================================================

    VSTLinkedParameterUI.cpp
    Created: 7 Dec 2020 7:01:39pm
    Author:  bkupe

  ==============================================================================
*/

#include "VSTLinkedParameterUI.h"

VSTParameterContainerEditor::VSTParameterContainerEditor(VSTParameterContainer* vstParamContainer, bool isRoot) :
    GenericControllableContainerEditor(vstParamContainer, isRoot),
    vstParamContainer(vstParamContainer)
{
    addParamBT.reset(AssetManager::getInstance()->getAddBT());
    addParamBT->addListener(this);
    addAndMakeVisible(addParamBT.get());
}

VSTParameterContainerEditor::~VSTParameterContainerEditor()
{
}

void VSTParameterContainerEditor::resizedInternalHeader(Rectangle<int>& r)
{
    GenericControllableContainerEditor::resizedInternalHeader(r);
    addParamBT->setBounds(r.removeFromRight(r.getHeight()).reduced(1));
}

void VSTParameterContainerEditor::buttonClicked(Button* b)
{
    if (b == addParamBT.get())
    {
        PopupMenu p;
        p.addItem(-1, "Add all");
        p.addItem(-2, "Remove all");
        p.addSeparator();

        const Array<AudioProcessorParameter *> &params = vstParamContainer->vst->getParameters();

        fillPopupMenuForGroup(&vstParamContainer->vst->getParameterTree(), p);

        if (int result = p.show())
        {
            if (result == -1) vstParamContainer->addAllParams();
            else if (result == -2) vstParamContainer->removeAllParams();
            else vstParamContainer->addVSTParam(params[result - 1]);
        }
    }
    else
    {
        GenericControllableContainerEditor::buttonClicked(b);
    }
}

void VSTParameterContainerEditor::fillPopupMenuForGroup(const AudioProcessorParameterGroup* group, PopupMenu& menu)
{
     const Array<const AudioProcessorParameterGroup*> groups = group->getSubgroups(false);
    for (auto& g : groups)
    {
        PopupMenu subMenu;
        fillPopupMenuForGroup(g, subMenu);
        menu.addSubMenu(g->getName(), subMenu);
    }
    
    menu.addSeparator();

    Array<AudioProcessorParameter*> params = group->getParameters(false);
    for (auto& vstP : params)
    {
        int index = vstP->getParameterIndex();
        menu.addItem(index + 1, vstP->getName(32), !vstParamContainer->hasParam(index));
    }

    
}
