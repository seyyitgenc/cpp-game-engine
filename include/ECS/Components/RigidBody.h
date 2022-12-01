#pragma once

#include "Component.h"

class RigidBody : public Component
{
public:
    RigidBody() = default;
    ~RigidBody() = default;
    void update(float &dt) override final
    {
        const Uint8 *state = SDL_GetKeyboardState(NULL);
        if (state[SDL_SCANCODE_W])
        {
            transform->position.y -= dt * linearVelocity.x;
        }
        if (state[SDL_SCANCODE_S])
        {
            transform->position.y += dt * linearVelocity.y;
        }
        if (state[SDL_SCANCODE_D])
            transform->position.x += dt * linearVelocity.y;

        if (state[SDL_SCANCODE_A])
            transform->position.x -= dt * linearVelocity.y;
    }
    bool init() final
    {
        transform = &entity->getComponent<Transform>();
        return true;
    };

private:
    vf2d linearVelocity = {0.5, 0.5};
    Transform *transform = nullptr;
};