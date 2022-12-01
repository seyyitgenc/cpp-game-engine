#pragma once
#include "SDL.h"
#include "ECS/Components/CollisionBox.h"

class Collider
{
public:
    static bool AABB(SDL_FRect &rectA, SDL_FRect &rectB)
    {
        bool intersect = rectA.x < rectB.x + rectB.w &&
                         rectA.x + rectA.w > rectB.x &&
                         rectA.y < rectB.y + rectB.h &&
                         rectA.h + rectA.y > rectB.y;
        return intersect;
    };
    static bool AABB(CollisionBox &colBoxA, CollisionBox &colBoxB)
    {
        return AABB(colBoxA.colRect, colBoxB.colRect);
    };
};