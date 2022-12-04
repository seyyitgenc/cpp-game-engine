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
        SDL_RenderCopyExF(rTarget, texture, nullptr, &dstRect, transform->rotation, nullptr, flip);
    }
    void update(float &dt) override final
    {
        dstRect.x = (transform->position.x);
        dstRect.y = (transform->position.y);
    }
    vi2d getSize() { return {width, height}; }

private:
    SDL_Renderer *rTarget = nullptr;
    std::string textureID = "";
    Transform *transform = nullptr;
    SDL_Texture *texture = nullptr;

    int width = 0;
    int height = 0;
    SDL_Rect *srcRect = nullptr;
    SDL_FRect dstRect = {0, 0, 0, 0};

    SDL_RendererFlip flip = SDL_FLIP_NONE;
};