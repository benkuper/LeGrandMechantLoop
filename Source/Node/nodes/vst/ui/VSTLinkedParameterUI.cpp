/*
  ==============================================================================

    VSTLinkedParameterUI.cpp
    Created: 7 Dec 2020 7:01:39pm
    Author:  bkupe

  ==============================================================================
*/

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

VSTLinkedParameterEditor::VSTLinkedParameterEditor(VSTParameterLink* pLink, bool isRoot) :
    ParameterEditor(pLink->param, isRoot),
    pLink(pLink)
{
    pLink->addAsyncVSTParameterLinkListener(this);
    linkBT.reset(AssetManager::getInstance()->getToggleBTImage(ImageCache::getFromMemory(BinaryData::link_png, BinaryData::link_pngSize)));
    addAndMakeVisible(linkBT.get());
    linkBT->addListener(this);
}

VSTLinkedParameterEditor::~VSTLinkedParameterEditor()
{
    pLink->removeAsyncVSTParameterLinkListener(this);
}

void VSTLinkedParameterEditor::paint(Graphics& g)
{
    Colour c = pLink->macroIndex == -1 ? NORMAL_COLOR : (pLink->macroIndex < pLink->maxMacros ? BLUE_COLOR : RED_COLOR);
    g.setColour(c);
    g.fillRoundedRectangle(linkBT->getBounds().withWidth(linkBT->getWidth()*2).toFloat(), 4);
}

void VSTLinkedParameterEditor::paintOverChildren(Graphics& g)
{
    if (pLink->macroIndex >= 0)
    {
        Rectangle<int> r = linkBT->getBounds().translated(linkBT->getWidth(), 0);
        g.setFont(10);
        g.setColour(Colours::white);
        g.drawText(String(pLink->macroIndex + 1), r.toFloat(), Justification::centred);
    }
}

void VSTLinkedParameterEditor::resizedInternal(Rectangle<int>& r)
{
    ParameterEditor::resizedInternal(r);
    r.removeFromRight(r.getHeight());
    linkBT->setBounds(r.removeFromRight(r.getHeight()).reduced(1));
}

void VSTLinkedParameterEditor::buttonClicked(Button* b)
{
    ParameterEditor::buttonClicked(b);
    if (b == linkBT.get())
    {
        PopupMenu m;
        for (int i = 0; i < pLink->maxMacros; i++)
        {
            m.addItem(i + 1, "Macro " + String(i + 1));
        }
        m.addItem(-1, "Unlink");

        if(int result = m.show())
        {

            if (result == -1) pLink->setMacroIndex(-1);
            else pLink->setMacroIndex(result - 1);
        }
    }
}

void VSTLinkedParameterEditor::newMessage(const VSTParameterLink::VSTParameterLinkEvent& e)
{
    repaint();
}
