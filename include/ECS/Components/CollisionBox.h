#pragma once
#include "Component.h"
#include "SDL.h"
#include "ECS/Components/Sprite.h"
#include "ECS/Components/Camera.h"

// TODO add isCentralized,isTop,isBottom for collision box position

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
        // bound collision rect to it's entity
        transform = &entity->getComponent<Transform>();
        colRect = {transform->position.x, transform->position.y, width, height};
        sprite = &entity->getComponent<Sprite>();
        spriteSize = sprite->getSize();
        return true;
    }
    void draw() override final
    {
        // // draw collision box
        cameraRect = playerCam.getCameraRect();
        bool intersect = cameraRect.x < colRect.x + colRect.w &&
                         cameraRect.x + cameraRect.w > colRect.x &&
                         cameraRect.y < colRect.y + colRect.h &&
                         cameraRect.y + cameraRect.h > colRect.y;
        if (intersect)
        {
            SDL_SetRenderDrawColor(rTarget, 255, 0, 0, 0);
            SDL_RenderDrawRectF(rTarget, &colRect);
        }
    }
    void update(float &dt) override final
    {
        // centralize collision box
        camPos = playerCam.getPos();
        colRect.x = transform->position.x - (colRect.w - spriteSize.x) / 2 - camPos.x;
        colRect.y = transform->position.y - (colRect.h - spriteSize.y) / 2 - camPos.y;
        position.x = colRect.x;
        position.y = colRect.y;
    }
    SDL_FRect getRect() { return colRect; }
    ~CollisionBox() = default;

private:
    friend class Collider; // collider can use this class
    SDL_Renderer *rTarget = nullptr;
    Transform *transform = nullptr; // for position
    Sprite *sprite = nullptr;       // for sprite size
    float width = 32.0f;
    float height = 32.0f;
    vi2d spriteSize = {32, 32};

    // for collision detection
    SDL_FRect colRect{0, 0, 0, 0};
    vf2d position = {0, 0};
    vf2d size = {32, 32};

    // camera
    vf2d camPos;
    SDL_FRect cameraRect;
};