/*
  ==============================================================================

    Interface.h
    Created: 15 Nov 2020 8:44:13am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

class Interface :
    public BaseItem
{
public:
    Interface(StringRef name = "Interface", var params = var());
    virtual ~Interface();

    BoolParameter* logIncomingData;
    BoolParameter* logOutgoingData;
};