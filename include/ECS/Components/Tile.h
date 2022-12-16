#pragma once
#include <fstream>
#include "Component.h"
#include "Engine/TileConfiguration.h"
class Tile : public Component
{
public:
    Tile(float x, float y, int tileType) : mType(tileType)
    {
        dstRect.x = x;
        dstRect.y = y;
        dstRect.w = TILE_WIDTH;
        dstRect.h = TILE_HEIGHT;
    };
    bool init() override final
    {
        transform = &entity->getComponent<Transform>();
        transform->position = vf2d{dstRect.x, dstRect.y};
        dstRect.w = dstRect.w * transform->scale.x;
        dstRect.h = dstRect.h * transform->scale.y;
        cameraRect = Camera::get().getCameraRect(); // camera rect
        return true;
    }
    void draw() override final
    {
        bool intersect = cameraRect.x < dstRect.x + dstRect.w &&
                         cameraRect.x + cameraRect.w > dstRect.x &&
                         cameraRect.y < dstRect.y + dstRect.h &&
                         cameraRect.y + cameraRect.h > dstRect.y;
        if (intersect)
        {
            TileConfiguration::get().tileTexture->render(dstRect.x, dstRect.y, &TileConfiguration::get().tileClips[mType]);
        }
    }
    void update(float &dt) override final
    {
        // update camera pos
        camPos = Camera::get().getPos();
        dstRect.x = transform->position.x - camPos.x;
        dstRect.y = transform->position.y - camPos.y;
    }

    vf2d getTilePos() { return vf2d(dstRect.x, dstRect.y); }
    int getTileType() { return mType; }

private:
    vf2d camPos = {30, 30};
    SDL_FRect cameraRect;
    Transform *transform;
    SDL_FRect dstRect;
    int mType;
};