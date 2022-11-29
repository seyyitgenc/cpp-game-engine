#pragma once

#include <string>
#include "SDL.h"
#include "ECS/Entity.h"
#include "Component.h"
#include "ECS/AssetManager.h"

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
        SDL_RenderCopyExF(rTarget, texture, &srcRect, &dstRect, transform->rotation, nullptr, flip);
    }
    void update(float &dt) override final
    {
        dstRect.x = (transform->position.x);
        dstRect.y = (transform->position.y);
        dstRect.w = (width * transform->scale.x);
        dstRect.h = (height * transform->scale.y);
        if (transform->position.x > 800 && transform->position.y > 600)
        {
            transform->position.x = 0;
            transform->position.y = 0;
        }
    }

private:
    SDL_Renderer *rTarget = nullptr;
    std::string textureID = "";
    Transform *transform = nullptr;
    SDL_Texture *texture = nullptr;

    int width = 0;
    int height = 0;
    SDL_Rect srcRect = {0, 0, 0, 0};
    SDL_FRect dstRect = {0, 0, 0, 0};

    SDL_RendererFlip flip = SDL_FLIP_NONE;
};