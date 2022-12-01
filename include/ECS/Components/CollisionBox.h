#pragma once
#include "Component.h"
#include "SDL.h"
#include "ECS/Components/Sprite.h"

class CollisionBox : public Component
{
public:
    CollisionBox(SDL_Renderer *target, float width, float height) : rTarget(target), width(width), height(height) {}
    bool init() override final
    {
        transform = &entity->getComponent<Transform>();
        // bound collision rect to it's entity
        colRect = {transform->position.x, transform->position.y, width, height};
        sprite = &entity->getComponent<Sprite>();
        spriteSize = sprite->getSize();
        return true;
    }
    void draw() override final
    {
        // draw col box
        SDL_SetRenderDrawColor(rTarget, 255, 0, 0, 0);
        SDL_RenderDrawRectF(rTarget, &colRect);
    }
    void update(float &dt) override final
    {
        // centralize collision box
        colRect.x = transform->position.x - (colRect.w - spriteSize.x) / 2;
        colRect.y = transform->position.y - (colRect.h - spriteSize.y) / 2;
    }
    SDL_FRect getRect() { return colRect; }
    ~CollisionBox() = default;

private:
    friend class Collider;
    float width = 32;
    float height = 32;
    SDL_Renderer *rTarget = nullptr;
    Transform *transform = nullptr;
    Sprite *sprite = nullptr;
    vi2d spriteSize;
    vf2d position = {0, 0};
    SDL_FRect colRect{0, 0, 0, 0};
};