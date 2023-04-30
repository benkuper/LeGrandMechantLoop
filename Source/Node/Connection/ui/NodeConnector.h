/*
  ==============================================================================

	NodeConnector.h
	Created: 16 Nov 2020 3:03:26pm
	Author:  bkupe

  ==============================================================================
*/

#pragma once

class BaseNodeViewUI;

class NodeConnector :
	public Component,
	public SettableTooltipClient
{
public:
	NodeConnector(BaseNodeViewUI* n, bool isInput, NodeConnection::ConnectionType connectionType);
	~NodeConnector();

	NodeConnection::ConnectionType connectionType;
	bool isInput;
	BaseNodeViewUI* nodeViewUI;


	void paint(Graphics& g) override;
	void update();

	void mouseDrag(const MouseEvent& e) override;
};