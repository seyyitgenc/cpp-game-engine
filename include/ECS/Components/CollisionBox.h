#pragma once
#include "Component.h"
#include "SDL.h"
#include "ECS/Components/Sprite.h"

class CollisionBox : public Component
{
public:
    CollisionBox() = default;
    CollisionBox(SDL_Renderer *target, float width, float height) : rTarget(target), width(width), height(height)
    {
        size = {width, height};
    }
    CollisionBox(float width, float height) : width(width), height(height)
    {
        size = {width, height};
    }
    bool init() override final
    {
        transform = &entity->getComponent<Transform>();
        // bound collision rect to it's entity
        colRect = {transform->position.x, transform->position.y, size.x, size.y};
        sprite = &entity->getComponent<Sprite>();
        spriteSize = sprite->getSize();
        return true;
    }
    void draw() override final
    {
        // draw collision box
        SDL_SetRenderDrawColor(rTarget, 255, 0, 0, 0);
        SDL_RenderDrawRectF(rTarget, &colRect);
    }
    void update(float &dt) override final
    {
        // centralize collision box
        colRect.x = transform->position.x - (colRect.w - spriteSize.x) / 2;
        colRect.y = transform->position.y - (colRect.h - spriteSize.y) / 2;
        position.x = colRect.x;
        position.y = colRect.y;
    }
    SDL_FRect getRect() { return colRect; }
    ~CollisionBox() = default;

private:
    friend class Collider;
    SDL_Renderer *rTarget = nullptr;
    Transform *transform = nullptr;
    Sprite *sprite = nullptr;
    float width = 32.0f;
    float height = 32.0f;
    vi2d spriteSize = {32, 32};
    vf2d size = {32, 32};
    vf2d position = {0, 0};

    SDL_FRect colRect{0, 0, 0, 0};
};