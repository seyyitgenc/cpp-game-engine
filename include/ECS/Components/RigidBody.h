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
    vf2d getVelocity() { return linearVelocity; }
    void setVelocity(vf2d velocity) { linearVelocity = velocity; }
    bool init() final
    {
        transform = &entity->getComponent<Transform>();
        return true;
    };
    vf2d linearVelocity = {0, 0};
private:
    float movementSpeed = 0.5;
    Transform *transform = nullptr;
};