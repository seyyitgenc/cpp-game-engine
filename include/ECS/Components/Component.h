#pragma once

#include "ECS/Entity.h"

class Component
{
public:
    // Specifies component entity
    Entity *entity;

    virtual bool init() { return true; }
    virtual void draw(){};
    virtual void update(float &dt){};

    ~Component() = default;
};
