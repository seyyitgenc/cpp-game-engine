#pragma once

// this Entity represents Entity class in Entity.h
#include "../Entity.h"

class Component
{
public:
    // Specifies component entity
    Entity *entity;

    virtual bool init() { return true; }
    virtual void draw(){};
    virtual void update(){};

    ~Component() = default;
};
