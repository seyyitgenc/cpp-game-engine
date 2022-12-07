#pragma once
#include <string>
#include "SDL.h"
#include "Component.h"
#include "ECS/Entity.h"
#include "ECS/AssetManager.h"

class Sprite : public Component
{
public:
    ~Sprite() = default;

    Sprite(SDL_Renderer *target, std::string textureID, int width, int height) : rTarget(target), textureID(textureID), width(width), height(height) {}

    bool init() override final
    {
        transform = &entity->getComponent<Transform>();
        texture = AssetManager::get().getTexture(textureID);

        dstRect.x = transform->position.x;
        dstRect.y = transform->position.y;
        dstRect.w = width * transform->scale.x;
        dstRect.h = height * transform->scale.y;
        return true;
    }
    void draw() override final
    {
        camPos = playerCam.getPos();
        camerRect = playerCam.getCameraRect();
        dstRect.x = transform->position.x - camPos.x;
        dstRect.y = transform->position.y - camPos.y;
        bool intersect = camerRect.x < dstRect.x + dstRect.w &&
                         camerRect.x + camerRect.w > dstRect.x &&
                         camerRect.y < dstRect.y + dstRect.h &&
                         camerRect.y + camerRect.h > dstRect.y;
        if (intersect)
        {

            SDL_RenderCopyExF(rTarget, texture, nullptr, &dstRect, 0.0f, nullptr, flip);
        }
    }
    void update(float &dt) override final
    {
    }
    vi2d getSize() { return {width, height}; }

private:
    vf2d camPos = {0, 0};
    SDL_Rect camerRect = {0, 0, 0, 0};
    SDL_Renderer *rTarget = nullptr;
    std::string textureID = "";
    Transform *transform = nullptr;
    SDL_Texture *texture = nullptr;

    int width = 0;
    int height = 0;
    SDL_FRect dstRect = {0, 0, 0, 0};

    SDL_RendererFlip flip = SDL_FLIP_NONE;
};