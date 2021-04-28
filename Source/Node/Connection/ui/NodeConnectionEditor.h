/*
  ==============================================================================

    NodeConnectionEditor.h
    Created: 16 Nov 2020 10:01:11am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

class NodeAudioConnectionEditor :
    public BaseItemEditor
{
public:
    NodeAudioConnectionEditor(NodeAudioConnection* connection, bool isRoot);
    ~NodeAudioConnectionEditor();

    NodeAudioConnection* connection;
    Rectangle<int> slotsRect;

    void resizedInternalContent(Rectangle<int> &r) override;

    void mouseDown(const MouseEvent& e) override;
    void mouseDoubleClick(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;

    class ChannelSlot :
        public Component
    {
    public:
        ChannelSlot(const String &channelName, int channel, bool isSource);
        ~ChannelSlot() {}

        int channel;
        bool isSource;
        Rectangle<int> slotRect;
        Rectangle<int> labelRect;

        void resized() override;
        void paint(Graphics& g) override;
        bool hitTest(int x, int y) override;
    };

    class ChannelConnection :
        public Component
    {
    public:
        ChannelConnection(ChannelSlot* sourceSlot, ChannelSlot* destSlot);
        ~ChannelConnection() {}

        ChannelSlot* sourceSlot;
        ChannelSlot* destSlot;

        Path path;
        Path hitPath;

        void updateBounds();
        void resized() override;
        void paint(Graphics& g) override;
        bool hitTest(int x, int y) override;

        void buildPath();

    };
    OwnedArray<ChannelSlot> sourceSlots;
    OwnedArray<ChannelSlot> destSlots;
    OwnedArray<ChannelConnection> connections;
    std::unique_ptr<ChannelConnection> tmpCreateConnection;
    ChannelSlot* tmpCreateSlot;

    void clearSlots();
    void buildSlots();
    void clearConnections();
    void buildConnections(bool resizeAfter = true);

    void startCreateConnection(ChannelSlot * startSlot);
    void updateCreateConnection();
    void endCreateConnection();

    ChannelSlot* getCandidateSlotAtMousePos();

    void addConnection(ChannelSlot* source, ChannelSlot* dest);
    void removeConnection(ChannelConnection* c, bool rebuild = true);

    ChannelConnection* getConnectionWithDest(ChannelSlot* dest);
    bool checkConnectionExists(ChannelSlot* source, ChannelSlot* dest);


};