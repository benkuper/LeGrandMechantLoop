/*
  ==============================================================================

	TransportUI.cpp
	Created: 15 Nov 2020 8:53:53am
	Author:  bkupe

  ==============================================================================
*/

#include "TransportUI.h"

TransportUI::TransportUI(StringRef name) :
	ShapeShifterContentComponent(name),
	InspectableContent(Transport::getInstance()),
	transport(Transport::getInstance())
{
	playBT.reset(transport->playTrigger->createImageUI(ImageCache::getFromMemory(BinaryData::play_png, BinaryData::play_pngSize)));
	addAndMakeVisible(playBT.get());

	stopBT.reset(transport->stopTrigger->createImageUI(ImageCache::getFromMemory(BinaryData::stop_png, BinaryData::stop_pngSize)));
	addAndMakeVisible(stopBT.get());

	beatsPerBarUI.reset(transport->beatsPerBar->createLabelUI());
	addAndMakeVisible(beatsPerBarUI.get());
	beatUnitUI.reset(transport->beatUnit->createLabelUI());
	addAndMakeVisible(beatUnitUI.get());
	
	bpmUI.reset(transport->bpm->createLabelParameter());
	addAndMakeVisible(bpmUI.get());

	quantizUI.reset(transport->quantization->createUI());
	addAndMakeVisible(quantizUI.get());

	curBarUI.reset(transport->curBar->createLabelUI());
	addAndMakeVisible(curBarUI.get());
	curBeatUI.reset(transport->curBeat->createLabelUI());
	addAndMakeVisible(curBeatUI.get());

	addAndMakeVisible(barViz);

	transport->addAsyncContainerListener(this);
}

TransportUI::~TransportUI()
{
	if (Transport::getInstanceWithoutCreating() != nullptr) transport->removeAsyncContainerListener(this);
}

void TransportUI::resized()
{
	Rectangle<int> r = getLocalBounds().reduced(2);

	Rectangle<int> barR = r.removeFromTop(20);
	curBeatUI->setBounds(barR.removeFromRight(30).reduced(2));
	curBarUI->setBounds(barR.removeFromRight(30).reduced(2));
	barViz.setBounds(barR.reduced(4));

	Rectangle<int> tr = r.removeFromBottom(20);
	playBT->setBounds(tr.removeFromLeft(tr.getHeight()).reduced(2));
	stopBT->setBounds(tr.removeFromLeft(tr.getHeight()).reduced(2));
	beatUnitUI->setBounds(tr.removeFromRight(30).reduced(2));
	beatsPerBarUI->setBounds(tr.removeFromRight(30).reduced(2));
	tr.removeFromRight(10);
	bpmUI->setBounds(tr.removeFromRight(80).reduced(2));

	Rectangle<int> qr = r.removeFromTop(20);
	quantizUI->setBounds(qr.removeFromRight(140).reduced(2));

}

void TransportUI::mouseDown(const MouseEvent& e)
{
	if (e.eventComponent == this) inspectable->selectThis();
}

void TransportUI::newMessage(const ContainerAsyncEvent& e)
{
	if (e.type == e.ControllableFeedbackUpdate)
	{
		if (e.targetControllable.wasObjectDeleted()) return;
		if (e.targetControllable == transport->beatsPerBar) barViz.rebuild();
		else if (e.targetControllable == transport->isCurrentlyPlaying) barViz.repaint();
	}
}

TransportUI::BeatViz::BeatViz(int index) :
    index(index),
    transport(Transport::getInstance())
{
}

void TransportUI::BeatViz::paint(Graphics& g)
{
	float beat = transport->curBeat->intValue();
	float progression = transport->beatProgression->floatValue();
	Colour c = NORMAL_COLOR;
	
	if (transport->isCurrentlyPlaying->boolValue())
	{
		c = beat >= index ? BLUE_COLOR.darker() : NORMAL_COLOR;
		if (beat == index) c = c.interpolatedWith(YELLOW_COLOR, 1 - progression);
	}

	g.setColour(c);
	g.fillRoundedRectangle(getLocalBounds().toFloat(), 2);
}

TransportUI::BarViz::BarViz() : 
	transport(Transport::getInstance())
{
	rebuild();
	startTimerHz(20);
}

void TransportUI::BarViz::rebuild()
{
	for (auto& b : beats) removeChildComponent(b);
	beats.clear();

	for (int i = 0; i < transport->beatsPerBar->intValue(); i++)
	{
		BeatViz* b = new BeatViz(i);
		addAndMakeVisible(b);
		beats.add(b);
	}

	BeatViz* b = new BeatViz(0);
	addAndMakeVisible(b);
	beats.add(b);

	resized();
}

void TransportUI::BarViz::paint(Graphics& g)
{
	Rectangle<int> r = getLocalBounds().reduced(5, 2);
	float progression = transport->barProgression->floatValue();
	Colour c = transport->isCurrentlyPlaying->boolValue() ? BLUE_COLOR : NORMAL_COLOR.darker();
	if (transport->isSettingTempo)
	{
		float diff = Time::getMillisecondCounter() / 1000.0 - transport->timeAtStart;
		c = BG_COLOR.darker(.2f).interpolatedWith(RED_COLOR, sinf(diff * 3 + float_Pi) * .5f + .5f);
		progression = 1;
	}
	g.setColour(BG_COLOR.darker(.2f));
	g.fillRoundedRectangle(r.toFloat(),2);
	g.setColour(c);
	g.fillRoundedRectangle(r.withWidth(r.getWidth() * progression).toFloat(), 2);
}

void TransportUI::BarViz::resized()
{
	Rectangle<int> r = getLocalBounds().reduced(5, 0);
	int beatGap = r.getWidth() / jmax(beats.size()-1,1);
	const int beatWidth = 4;
	r.setWidth(beatWidth);

	for (int i = 0; i < beats.size(); i++) beats[i]->setBounds(r.translated(i * beatGap, 0));
}

void TransportUI::BarViz::timerCallback()
{
	if(transport->isCurrentlyPlaying->boolValue() || transport->isSettingTempo) repaint();
}
