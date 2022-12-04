#pragma once
#include "Component.h"

class RigidBody : public Component
{
public:
    RigidBody() = default;
    ~RigidBody() = default;
    void update(float &dt) override final
    {
        // TODO this part must have physic calculations
        // linearVelocity = {0, 0};
        // const Uint8 *state = SDL_GetKeyboardState(NULL);
        // if (state[SDL_SCANCODE_W])
        // {
        //     linearVelocity.y -= movementSpeed;
        // }
        // if (state[SDL_SCANCODE_S])
        // {
        //     linearVelocity.y += movementSpeed;
        // }
        // if (state[SDL_SCANCODE_D])
        // {
        //     linearVelocity.x += movementSpeed;
        // }

        // if (state[SDL_SCANCODE_A])
        // {
        //     linearVelocity.x -= movementSpeed;
        // }
        // transform->position += linearVelocity * dt;
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