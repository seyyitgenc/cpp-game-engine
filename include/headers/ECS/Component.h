#pragma once

// this Entity represents Entity class in Entity.h
#include "Entity.h"

class Component
{
public:
    // default means you don't have to initialize
    Component() = default;
    ~Component() = default;
    // Specifies component entity
    Entity *entity;

    virtual bool init() { return true; }
    virtual void draw(){};
    virtual void update(){};
};
