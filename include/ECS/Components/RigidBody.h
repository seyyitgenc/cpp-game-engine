#pragma once

#include "Component.h"

class RigidBody : public Component
{
public:
    RigidBody() = default;
    ~RigidBody() = default;
    void update(float &dt) override final
    {
        
    }
    bool init() final
    {
        t1 = &entity->getComponent<Transform>();
        return true;
    };

private:
    Transform *t1 = nullptr;
};