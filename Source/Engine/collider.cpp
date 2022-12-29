#include "collider.h"

bool Collider::AABB(const SDL_FRect &rectA, const SDL_FRect &rectB) {
    return rectA.x < rectB.x + rectB.w && rectA.x + rectA.w > rectB.x && rectA.y < rectB.y + rectB.h &&
           rectA.y + rectA.h > rectB.y;
}

// SWEPT AABB CODE BY OLC
bool Collider::RayVsRect(const vf2d &ray_origin, const vf2d &ray_dir, const SDL_FRect *target, vf2d &contact_point,
                         vf2d &contact_normal, float &t_hit_near) {
    contact_normal = {0, 0};
    contact_point = {0, 0};
    vf2d pos = {target->x, target->y};
    vf2d size = {target->w, target->h};
    // Calculate intersections with rectangle bounding axes
    vf2d t_near = (pos - ray_origin) / ray_dir;
    vf2d t_far = (pos + size - ray_origin) / ray_dir;

    if (std::isnan(t_far.y) || std::isnan(t_far.x)) return false;
    if (std::isnan(t_near.y) || std::isnan(t_near.x)) return false;

    // Sort distances
    if (t_near.x > t_far.x) std::swap(t_near.x, t_far.x);
    if (t_near.y > t_far.y) std::swap(t_near.y, t_far.y);

    // Early rejection
    if (t_near.x > t_far.y || t_near.y > t_far.x) return false;

    // Closest 'time' will be the first contact
    t_hit_near = std::max(t_near.x, t_near.y);

    // Furthest 'time' is contact on opposite side of target
    float t_hit_far = std::min(t_far.x, t_far.y);

    // Reject if ray direction is pointing away from object
    if (t_hit_far < 0) return false;

    // Contact point of collision from parametric line equation
    contact_point = ray_origin + ray_dir * t_hit_near;
    if (t_near.x > t_near.y)
        if (ray_dir.x < 0)
            contact_normal = {1, 0};
        else
            contact_normal = {-1, 0};
    else if (t_near.x < t_near.y)
        if (ray_dir.y < 0)
            contact_normal = {0, 1};
        else
            contact_normal = {0, -1};

    return true;
}

bool Collider::DynamicRectVsRect(const CollisionBox *r_dynamic, const float fTimeStep, const CollisionBox &r_static,
                                 vf2d &contact_point, vf2d &contact_normal, float &contact_time) {
    // Check if dynamic rectangle is actually moving - we assume rectangles are
    // NOT in collision to start
    if (velocity.x == 0 && velocity.y == 0) return false;

    // Expand target rectangle by source dimensions
    SDL_FRect expanded_target = {
        r_static.position.x - r_dynamic->size.x / 2,
        r_static.position.y - r_dynamic->size.y / 2,
        r_static.size.x + r_dynamic->size.x,
        r_static.size.y + r_dynamic->size.y,
    };
    SDL_SetRenderDrawColor(gRenderer, 255, 255, 0, 255);
    SDL_RenderDrawRectF(gRenderer, &expanded_target);
    if (RayVsRect(r_dynamic->position + r_dynamic->size / 2, velocity * fTimeStep, &expanded_target, contact_point,
                  contact_normal, contact_time)) {
        return (contact_time >= 0.0f && contact_time < 1.0f);
    } else
        return false;
}

bool Collider::ResolveDynamicRectVsRect(CollisionBox *r_dynamic, const float fTimeStep, CollisionBox *r_static) {
    vf2d contact_point, contact_normal;
    float contact_time = 0.0f;
    if (DynamicRectVsRect(r_dynamic, fTimeStep, *r_static, contact_point, contact_normal, contact_time)) {
        velocity += contact_normal * vf2d(std::abs(velocity.x), std::abs(velocity.y)) * (1 - contact_time);
        return true;
    }
    return false;
}
// TODO: add window collision
void Collider::resolveSweptAABB(std::vector<Entity *> &vrects, const float &dt) {
    velocity = vrects[0]->getComponent<RigidBody>().getVel();
    vf2d cp, cn;
    float t = 0;
    std::vector<std::pair<int, float>> z;
    for (size_t i = 1; i < vrects.size(); i++) {
        if (DynamicRectVsRect(&vrects[0]->getComponent<CollisionBox>(), dt, vrects[i]->getComponent<CollisionBox>(), cp, cn, t)) {
            z.push_back({i, t});
        }
    }
    std::sort(z.begin(), z.end(),
              [](const std::pair<int, float> &a, const std::pair<int, float> &b) { return a.second < b.second; });
    // Now resolve the collision in correct order
    for (auto j : z) {
        ResolveDynamicRectVsRect(&vrects[0]->getComponent<CollisionBox>(), dt, &vrects[j.first]->getComponent<CollisionBox>());
    }
    if (vrects[0]->getComponent<Transform>().position.x < 0) vrects[0]->getComponent<Transform>().position.x = 0;
    if (vrects[0]->getComponent<Transform>().position.y < 0) vrects[0]->getComponent<Transform>().position.y = 0;
    if (vrects[0]->getComponent<Transform>().position.x + vrects[0]->getComponent<Sprite>().getSize().x > LEVEL_WIDTH)
        vrects[0]->getComponent<Transform>().position.x = LEVEL_WIDTH - vrects[0]->getComponent<Sprite>().getSize().x;
    if (vrects[0]->getComponent<Transform>().position.y + vrects[0]->getComponent<Sprite>().getSize().y > LEVEL_HEIGHT)
        vrects[0]->getComponent<Transform>().position.y = LEVEL_HEIGHT - vrects[0]->getComponent<Sprite>().getSize().y;
    vrects[0]->getComponent<Transform>().position += velocity * dt;
}