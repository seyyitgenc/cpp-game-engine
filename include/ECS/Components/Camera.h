#pragma once
#include "Component.h"
#include "Sprite.h"
#include "SDL.h"


// class Camera : public Component
// {
// public:
//     Camera(int xoffset, int yoffset, int width, int height)
//     {
//         camera_rect =
//             {xoffset,
//              yoffset,
//              width,
//              height};
//     }
//     ~Camera() = default;

//     bool init()
//     {
//         // initialize camera rect

//         transform = &entity->getComponent<Transform>();
//         return true;
//     }
//     void update(float &dt) override final
//     {
//         // this should centralize camera for player
//         camera_rect.x = transform->position.x + camera_rect.w / 2 + 64 / 2;
//         camera_rect.y = transform->position.y + camera_rect.h / 2 + 64 / 2;
//     }
//     SDL_Rect getCameraRect() { return camera_rect; };

// private:
//     SDL_Rect camera_rect;
//     Transform *transform;
//     vi2d spriteSize;
// };