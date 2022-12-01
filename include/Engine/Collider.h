#pragma once
#include "SDL.h"
#include "ECS/Components/CollisionBox.h"

class Collider : public Component
{
public:
    Collider() = default;
    ~Collider() = default;

    bool init() override final
    {
        transform = &entity->getComponent<Transform>(); // for pos
        return true;
    }
    void resolveAABB(CollisionBox &colBoxA, CollisionBox &colBoxB);

private:
    SDL_FRect rectA;
    SDL_FRect rectB;
    Transform *transform;
};

void Collider::resolveAABB(CollisionBox &colBoxA, CollisionBox &colBoxB)
{
    rectA = colBoxA.colRect;
    rectB = colBoxB.colRect;
    bool intersect = rectA.x < rectB.x + rectB.w &&
                     rectA.x + rectA.w > rectB.x &&
                     rectA.y < rectB.y + rectB.h &&
                     rectA.h + rectA.y > rectB.y;
    if (intersect)
    {
        // TODO must find better solition
        if (rectA.x > rectB.x && rectA.x + rectA.w > rectB.x + rectB.w && rectA.y < rectB.y + rectB.h && rectA.y + rectA.h > rectB.y)
        {
            entity->getComponent<Transform>().position.x = rectB.x + rectB.w;
        }
        if (rectA.x < rectB.x && rectA.x + rectA.w < rectB.x + rectB.w && rectA.y < rectB.y + rectB.h && rectA.y + rectA.h > rectB.y)
        {
            entity->getComponent<Transform>().position.x = rectB.x - rectA.w;
        }
        if (rectA.y > rectB.y && rectA.y + rectA.h > rectB.y + rectB.h && rectA.x < rectB.x + rectB.w && rectA.x + rectA.x + rectA.w > rectB.x)
        {
            entity->getComponent<Transform>().position.y = rectB.y + rectB.h;
        }
        if (rectA.y < rectB.y && rectA.y + rectA.h < rectB.y + rectB.h && rectA.x < rectB.x + rectB.w && rectA.x + rectA.x + rectA.w > rectB.x)
        {
            entity->getComponent<Transform>().position.y = rectB.y - rectA.h;
        }
    }
}