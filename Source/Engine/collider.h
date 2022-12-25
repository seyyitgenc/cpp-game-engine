#pragma once
#include <SDL.h>
#include "ECS/entity.h"
#include "ECS/Components/components.h"
#include <algorithm>

class Collider
{
public:
    Collider() = default;
    ~Collider() = default;
    // SWEPT AABB
    void resolveSweptAABB(std::vector<Entity *> &vrects, float &dt);
    // Normal AABB
    bool AABB(const SDL_FRect &rectA, const SDL_FRect &rectB);

private:
    vf2d velocity = {0, 0};
    bool RayVsRect(const vf2d &ray_origin, const vf2d &ray_dir, const SDL_FRect *target, vf2d &contact_point, vf2d &contact_normal, float &t_hit_near);
    bool DynamicRectVsRect(const CollisionBox *r_dynamic, const float fTimeStep, const CollisionBox &r_static, vf2d &contact_point, vf2d &contact_normal, float &contact_time);
    bool ResolveDynamicRectVsRect(CollisionBox *r_dynamic, const float fTimeStep, CollisionBox *r_static);
};