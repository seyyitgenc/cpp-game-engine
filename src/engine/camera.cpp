// #include "Engine/camera.h"

// #include "globals.h"

// Camera *Camera::s_instance;

// Camera &Camera::get() {
//     if (s_instance == nullptr) {
//         std::cout << "Camera Initialized" << std::endl;
//         s_instance = new Camera(0.f, 0.f, SCREEN_WIDTH, SCREEN_HEIGHT);
//     }
//     return *s_instance;
// }

// Camera::Camera(float xoffset, float yoffset, float width, float height) {
//     camera = {xoffset, yoffset, width, height};
// }

// void Camera::setPos(const float &posx, const float &posy) {
//     x = (posx)-SCREEN_WIDTH / 2.0f;
//     y = (posy)-SCREEN_HEIGHT / 2.0f;
//     if (x < 0) x = 0;
//     if (y < 0) y = 0;
//     if (x > LEVEL_WIDTH - camera.w) x = LEVEL_WIDTH - camera.w;
//     if (y > LEVEL_HEIGHT - camera.h) y = LEVEL_HEIGHT - camera.h;
// }
// vf2d Camera::getPos() { return vf2d(x, y); }
// SDL_FRect &Camera::getCameraRect() { return camera; }