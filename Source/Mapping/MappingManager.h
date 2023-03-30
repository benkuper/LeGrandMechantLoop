/*
  ==============================================================================

    MappingManager.h
    Created: 3 May 2021 10:23:44am
    Author:  bkupe

  ==============================================================================
*/

#pragma once

#include "Mapping.h"

class MappingManager :
    public BaseManager<Mapping>
{
public:
    juce_DeclareSingleton(MappingManager, true);

    MappingManager();
    ~MappingManager();

    Factory<Mapping> factory;


    void createMappingForControllable(Controllable* c, const String& type);

    Mapping* getMappingForDestControllable(Controllable* c);
};