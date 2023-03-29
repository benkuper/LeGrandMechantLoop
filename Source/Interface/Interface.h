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

    //Do not include in hierarchy to avoid going crazy on those listeners
    std::unique_ptr<Trigger> inActivityTrigger;
    std::unique_ptr<Trigger> outActivityTrigger;
};