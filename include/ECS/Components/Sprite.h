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

    Sprite(SDL_Renderer *target, std::string textureID, int width, int height) : rTarget(target), textureID(textureID), width(width), height(height)
    {
    }
    bool init() override final
    {
        transform = &entity->getComponent<Transform>();
        texture = AssetManager::get().getTexture(textureID);

        // TODO use scale to scale up the image don't use specific width and height
        texture->setWidth(width);
        texture->setHeight(height);

        width = texture->getWidth();
        height = texture->getHeight();

        dstRect.x = transform->position.x;
        dstRect.y = transform->position.y;
        dstRect.w = width * transform->scale.x;
        dstRect.h = height * transform->scale.y;
        cameraRect = Camera::get().getCameraRect();
        return true;
    }
    void update(float &dt) override final
    {
        camPos = Camera::get().getPos();
        dstRect.x = transform->position.x - camPos.x;
        dstRect.y = transform->position.y - camPos.y;
    }
    void draw() override final
    {

        bool intersect = cameraRect.x < dstRect.x + dstRect.w &&
                         cameraRect.x + cameraRect.w > dstRect.x &&
                         cameraRect.y < dstRect.y + dstRect.h &&
                         cameraRect.y + cameraRect.h > dstRect.y;
        if (intersect)
        {
            texture->render(dstRect.x, dstRect.y);
        }
    }
    vi2d getSize() { return {width, height}; }

private:
    vf2d camPos = {0, 0};
    SDL_FRect cameraRect = {0, 0, 0, 0};
    SDL_Renderer *rTarget = nullptr;
    std::string textureID = "";
    Transform *transform = nullptr;
    Texture *texture = nullptr;

    int width = 0;
    int height = 0;
    SDL_FRect dstRect = {0, 0, 0, 0};

    SDL_RendererFlip flip = SDL_FLIP_NONE;
};