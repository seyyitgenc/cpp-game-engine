#pragma once
#include "entity.h"

class Component {
public:
    // Specifies component entity
    Entity *entity;

    virtual bool init() { return true; }
    virtual void draw(){};
    virtual void update(const float &dt){};

    ~Component() = default;
};