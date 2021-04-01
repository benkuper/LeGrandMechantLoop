/*
  ==============================================================================

    NodeAudioConnectionEditor.cpp
    Created: 16 Nov 2020 10:01:11am
    Author:  bkupe

  ==============================================================================
*/

#include "NodeConnectionEditor.h"
#include "Node/Node.h"
#include "Common/ConnectionUIHelper.h"

NodeAudioConnectionEditor::NodeAudioConnectionEditor(NodeAudioConnection* connection, bool isRoot) :
    BaseItemEditor(connection, isRoot),
    connection(connection),
    tmpCreateSlot(nullptr)
{
    buildSlots();
    buildConnections();

    setInterceptsMouseClicks(true, true);
    removeMouseListener(this);
    addMouseListener(this, true);
    setDisableDefaultMouseEvents(true);
}

NodeAudioConnectionEditor::~NodeAudioConnectionEditor()
{

}

void NodeAudioConnectionEditor::resizedInternalContent(Rectangle<int>& r)
{
    const int slotHeight = 30;
    const int maxSlots = jmax(sourceSlots.size(), destSlots.size());
    int th = maxSlots * slotHeight;
    slotsRect = r.withHeight(th);
    r.setY(slotsRect.getBottom());


    for (int i = 0; i < maxSlots; i++)
    {
        Rectangle<int> sr = slotsRect.removeFromTop(slotHeight);
        int tw = sr.getWidth() / 3;
        if (i < sourceSlots.size()) sourceSlots[i]->setBounds(sr.removeFromLeft(jlimit(30, 200, tw)));
        if (i < destSlots.size()) destSlots[i]->setBounds(sr.removeFromRight(jlimit(30, 200, tw)));
    }

    for (auto& c : connections) c->updateBounds();
}

void NodeAudioConnectionEditor::mouseDown(const MouseEvent& e)
{
    if (ChannelConnection* c = dynamic_cast<ChannelConnection*>(e.originalComponent))
    {
        if (e.mods.isRightButtonDown())
        {
            PopupMenu p;
            p.addItem(1, "Disconnect");

            if (int result = p.show())
            {
                switch (result)
                {
                case 1: removeConnection(c);  break;
                }
            }
        }
        else if (e.mods.isLeftButtonDown() && e.mods.isCommandDown())
        {
            removeConnection(c);
        }
    }
    else if (ChannelSlot* s = dynamic_cast<ChannelSlot*> (e.originalComponent))
    {
        startCreateConnection(s);
    }
    else if (e.eventComponent == this)
    {
        BaseItemEditor::mouseDown(e);
    }
}

void NodeAudioConnectionEditor::mouseDoubleClick(const MouseEvent& e)
{
    if (ChannelConnection* c = dynamic_cast<ChannelConnection*>(e.originalComponent))
    {
        if (e.mods.isLeftButtonDown())
        {
            removeConnection(c);
        }
    }
}

void NodeAudioConnectionEditor::mouseDrag(const MouseEvent& e)
{
    if (ChannelSlot* s = dynamic_cast<ChannelSlot*> (e.originalComponent))
    {
        updateCreateConnection();
    }
    else if (e.eventComponent == this)
    {
        BaseItemEditor::mouseDrag(e);
    }
}

void NodeAudioConnectionEditor::mouseUp(const MouseEvent& e)
{
    if (ChannelSlot* s = dynamic_cast<ChannelSlot*> (e.originalComponent))
    {
        endCreateConnection();
    }
    else if (e.eventComponent == this)
    {
        BaseItemEditor::mouseUp(e);
    }
}

void NodeAudioConnectionEditor::clearSlots()
{
    for (auto& c : sourceSlots) removeChildComponent(c);
    for (auto& c : destSlots) removeChildComponent(c);

    sourceSlots.clear();
    destSlots.clear();
}

void NodeAudioConnectionEditor::buildSlots()
{
    jassert(connection->sourceNode != nullptr && connection->destNode != nullptr);

    clearSlots();

    int numOutputs = connection->sourceNode->getNumAudioOutputs();
    int numInputs = connection->destNode->getNumAudioInputs();

    for (int i = 0; i < numOutputs; i++)
    {
        ChannelSlot* s = new ChannelSlot(connection->sourceNode->audioOutputNames[i], i, true);
        sourceSlots.add(s);
        addAndMakeVisible(s);
    }

    for (int i = 0; i < numInputs; i++)
    {
        ChannelSlot* s = new ChannelSlot(connection->destNode->audioInputNames[i], i, false);
        destSlots.add(s);
        addAndMakeVisible(s);
    }
}

void NodeAudioConnectionEditor::clearConnections()
{
    for (auto& c : connections) removeChildComponent(c);
    connections.clear();
}

void NodeAudioConnectionEditor::buildConnections(bool resizeAfter)
{
    clearConnections();
    for (auto& co : connection->channelMap)
    {
        ChannelConnection* c = new ChannelConnection(sourceSlots[co.sourceChannel], destSlots[co.destChannel]);
        addAndMakeVisible(c);
        connections.add(c);
    }

    if (resizeAfter) resized();
}

void NodeAudioConnectionEditor::startCreateConnection(ChannelSlot* startSlot)
{
    tmpCreateConnection.reset(new ChannelConnection(startSlot->isSource ? startSlot : nullptr, startSlot->isSource ? nullptr : startSlot));
    tmpCreateSlot = startSlot;

    addAndMakeVisible(tmpCreateConnection.get());
    tmpCreateConnection->updateBounds();
}

void NodeAudioConnectionEditor::updateCreateConnection()
{
    if (tmpCreateConnection == nullptr)  return;
    ChannelSlot* s = getCandidateSlotAtMousePos();

    if (tmpCreateSlot->isSource) tmpCreateConnection->destSlot = s;
    else tmpCreateConnection->sourceSlot = s;

    tmpCreateConnection->updateBounds();
}

void NodeAudioConnectionEditor::endCreateConnection()
{
    if (tmpCreateConnection != nullptr && tmpCreateConnection->sourceSlot != nullptr && tmpCreateConnection->destSlot != nullptr)
    {
        addConnection(tmpCreateConnection->sourceSlot, tmpCreateConnection->destSlot);
    }

    removeChildComponent(tmpCreateConnection.get());
    tmpCreateConnection.reset();
    tmpCreateSlot = nullptr;
}

NodeAudioConnectionEditor::ChannelSlot* NodeAudioConnectionEditor::getCandidateSlotAtMousePos()
{
    if (tmpCreateSlot == nullptr) return nullptr;

    Point<int> pos = getMouseXYRelative();

    OwnedArray<ChannelSlot>* slotArray = tmpCreateSlot->isSource ? &destSlots : &sourceSlots;
    for (auto& s : *slotArray)
    {
        if (s->getBounds().contains(pos)) return s;
    }
    return nullptr;
}


void NodeAudioConnectionEditor::addConnection(ChannelSlot* source, ChannelSlot* dest)
{
    if (checkConnectionExists(source, dest))
    {
        LOGWARNING("Connection already exists, not creating");
        return;
    }

    //if (ChannelConnection* c = getConnectionWithDest(dest))  removeConnection(c, false);

    connection->connectChannels(source->channel, dest->channel);

    buildConnections();
}

void NodeAudioConnectionEditor::removeConnection(ChannelConnection* c, bool rebuild)
{
    connection->disconnectChannels(c->sourceSlot->channel, c->destSlot->channel);

    if (rebuild)  buildConnections();
}

NodeAudioConnectionEditor::ChannelConnection* NodeAudioConnectionEditor::getConnectionWithDest(ChannelSlot* dest)
{
    for (auto& c : connections) if (c->destSlot == dest) return c;
    return nullptr;
}

bool NodeAudioConnectionEditor::checkConnectionExists(ChannelSlot* source, ChannelSlot* dest)
{
    if (ChannelConnection* c = getConnectionWithDest(dest)) return c->sourceSlot == source;
    return false;
}


NodeAudioConnectionEditor::ChannelSlot::ChannelSlot(const String& channelName, int channel, bool isSource) :
    Component(channelName),
    channel(channel),
    isSource(isSource)
{
    setRepaintsOnMouseActivity(true);
}


void NodeAudioConnectionEditor::ChannelSlot::resized()
{
    labelRect = getLocalBounds();
    slotRect = isSource ? labelRect.removeFromRight(getHeight()) : labelRect.removeFromLeft(getHeight());
}

void NodeAudioConnectionEditor::ChannelSlot::paint(Graphics& g)
{
    g.setColour(TEXT_COLOR.darker(.3f));
    g.drawText(getName(), labelRect.toFloat().reduced(2), isSource ? Justification::centredRight : Justification::centredLeft);

    Colour c = isMouseOverOrDragging() ? HIGHLIGHT_COLOR : TEXT_COLOR;
    g.setColour(c);
    g.fillEllipse(slotRect.withSizeKeepingCentre(8, 8).toFloat());
}

bool NodeAudioConnectionEditor::ChannelSlot::hitTest(int x, int y)
{
    return slotRect.contains(x, y);
}

NodeAudioConnectionEditor::ChannelConnection::ChannelConnection(ChannelSlot* sourceSlot, ChannelSlot* destSlot) :
    sourceSlot(sourceSlot),
    destSlot(destSlot)
{
    updateBounds();
    setRepaintsOnMouseActivity(true);
}

void NodeAudioConnectionEditor::ChannelConnection::updateBounds()
{
    if ((sourceSlot == nullptr && destSlot == nullptr) || getParentComponent() == nullptr) return;

    Rectangle<float> mouseR = Rectangle<int>(0, 0, 10, 10).withCentre(getParentComponent()->getMouseXYRelative()).toFloat();
    Rectangle<float> sourceR = sourceSlot == nullptr ? mouseR : getParentComponent()->getLocalArea(sourceSlot, sourceSlot->slotRect.toFloat());
    Rectangle<float> destR = destSlot == nullptr ? mouseR : getParentComponent()->getLocalArea(destSlot, destSlot->slotRect.toFloat());

    Rectangle<int> r = sourceR.getUnion(destR).expanded(10).toNearestInt();
    setBounds(r);
    buildPath();
    resized();
}

void NodeAudioConnectionEditor::ChannelConnection::resized()
{
    buildPath();
}

void NodeAudioConnectionEditor::ChannelConnection::paint(Graphics& g)
{
    Colour c = (sourceSlot == nullptr || destSlot == nullptr) ? YELLOW_COLOR : TEXT_COLOR;
    if (isMouseOverOrDragging()) c = c.brighter();
    g.setColour(c);
    g.strokePath(path, PathStrokeType(isMouseOverOrDragging() ? 3 : 1));
}

bool NodeAudioConnectionEditor::ChannelConnection::hitTest(int x, int y)
{
    return hitPath.contains(x, y);
}

void NodeAudioConnectionEditor::ChannelConnection::buildPath()
{
    Point<float> mouseP = Rectangle<int>(0, 0, 10, 10).withCentre(getMouseXYRelative()).getCentre().toFloat();
    Point<float> sourceP = sourceSlot == nullptr ? mouseP : getLocalPoint(sourceSlot, sourceSlot->slotRect.getCentre()).toFloat();
    Point<float> destP = destSlot == nullptr ? mouseP : getLocalPoint(destSlot, destSlot->slotRect.getCentre()).toFloat();

    path.clear();
    path.startNewSubPath(sourceP);
    path.cubicTo(sourceP.translated(60, 0), destP.translated(-60, 0), destP);

    hitPath = PathHelpers::buildHitPath(&path);
}
