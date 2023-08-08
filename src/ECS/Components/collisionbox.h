// #pragma once
// #include "../component.h"
// #include "sprite.h"
// #include "tile.h"
// #include "transform.h"
// // TODO add isCentralized,isTop,isBottom for collision box position

// class CollisionBox : public Component {
// public:
//     CollisionBox() = default;

//     CollisionBox(SDL_Renderer *target, float width, float height)
//         : rTarget(target), width(width), height(height) {
//         size = {width, height};
//     }

//     CollisionBox(float width, float height) : width(width), height(height) {
//         size = {width, height};
//     }

//     bool init() override final {
//         // bound collision rect to it's entity
//         transform = &entity->getComponent<Transform>();
//         colRect = {transform->position.x, transform->position.y,
//                    width * transform->scale.x, height * transform->scale.y};
//         if (entity->hasComponent<Sprite>()) {
//             sprite = &entity->getComponent<Sprite>();
//             spriteSize = sprite->getSize();
//         } else if (entity->hasComponent<Tile>()) {
//             // TODO : check if it's tile get tileWidth and tileHeight
//             spriteSize = {width * transform->scale.x,
//                           height * transform->scale.y};
//         }
//         return true;
//     }

//     void draw() override final {
//         // draw collision box
//         cameraRect = Camera::get().getCameraRect();
//         bool intersect = cameraRect.x < colRect.x + colRect.w &&
//                          cameraRect.x + cameraRect.w > colRect.x &&
//                          cameraRect.y < colRect.y + colRect.h &&
//                          cameraRect.y + cameraRect.h > colRect.y;
//         if (intersect) {
//             SDL_SetRenderDrawColor(rTarget, 255, 0, 0, 255);
//             SDL_RenderDrawRectF(rTarget, &colRect);
//         }
//     }

//     void update(const float &dt) override final {
//         // centralize collision box
//         camPos = Camera::get().getPos();
//         colRect.x =
//             transform->position.x - (colRect.w - spriteSize.x) / 2 - camPos.x;
//         colRect.y =
//             transform->position.y - (colRect.h - spriteSize.y) / 2 - camPos.y;
//         position.x = colRect.x;
//         position.y = colRect.y;
//     }
//     SDL_FRect getRect() { return colRect; }
//     ~CollisionBox() = default;

// private:
//     // collisin box
//     friend class Collider;  // collider can use this class
//     SDL_Renderer *rTarget = nullptr;
//     Transform *transform = nullptr;  // for position
//     Sprite *sprite = nullptr;        // for sprite size
//     float width = 32.0f;
//     float height = 32.0f;
//     vf2d spriteSize = {32, 32};
//     bool isCentralized = false;

// private:
//     // for collision detection
//     SDL_FRect colRect{0, 0, 0, 0};
//     vf2d position = {0, 0};
//     vf2d size = {32, 32};

// private:
//     // camera
//     vf2d camPos;
//     SDL_FRect cameraRect;
// };