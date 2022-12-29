#pragma once
#include "../component.h"
#include "transform.h"

class RigidBody : public Component {
public:
    RigidBody() = default;
    ~RigidBody() = default;
    bool init() final {
        transform = &entity->getComponent<Transform>();
        return true;
    };
    void update(const float &dt) override final {
        linearVelocity = {0, 0};
        const Uint8 *state = SDL_GetKeyboardState(NULL);
        if (state[SDL_SCANCODE_W]) {
            linearVelocity.y -= movementSpeed;
        }
        if (state[SDL_SCANCODE_S]) {
            linearVelocity.y += movementSpeed;
        }
        if (state[SDL_SCANCODE_D]) {
            linearVelocity.x += movementSpeed;
        }
        if (state[SDL_SCANCODE_A]) {
            linearVelocity.x -= movementSpeed;
        }
        // position will be set on Collider.h
    }
    vf2d getVel() { return linearVelocity; }
    void setVel(float velx, float vely) { linearVelocity = vf2d(velx, vely); }

private:
    vf2d linearVelocity = {0, 0};
    float movementSpeed = 500.0f;
    Transform *transform = nullptr;
};