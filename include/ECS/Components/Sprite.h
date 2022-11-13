#pragma once

#include <string>
#include "../../SDL2/SDL.h"
#include "../Entity/Entity.h"
#include "../Component.h"
#include "../../Managers/AssetManager.h"

class Sprite : public Component
{
public:
    ~Sprite() = default;

    Sprite(SDL_Renderer *target, std::string textureID) : rTarget(target), textureID(textureID) {}

    bool init() override final
    {
        transform = &entity->getComponent<Transform>();
        texture = AssetManager::get().getTexture(textureID);
        SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);

        dstRect.x = transform->position.x;
        dstRect.y = transform->position.y;
        dstRect.w = width * transform->scale.x;
        dstRect.h = height * transform->scale.y;

        srcRect.x = 0;
        srcRect.y = 0;
        srcRect.w = width;
        srcRect.h = height;
        return true;
    }
    // TODO : Change rects to float rect
    void draw() override final
    {
        SDL_RenderCopyEx(rTarget, texture, &srcRect, &dstRect, transform->rotation, nullptr, flip);
    }
    void update() override final
    {
        dstRect.x = (int)(transform->position.x);
        dstRect.y = (int)(transform->position.y);
        dstRect.w = (int)(width * transform->scale.x);
        dstRect.h = (int)(height * transform->scale.y);
    }

private:
    SDL_Renderer *rTarget = nullptr;
    std::string textureID = "";
    Transform *transform = nullptr;
    SDL_Texture *texture = nullptr;

    int width = 0;
    int height = 0;
    SDL_Rect srcRect = {0, 0, 0, 0};
    SDL_Rect dstRect = {0, 0, 0, 0};

    SDL_RendererFlip flip = SDL_FLIP_NONE;
};