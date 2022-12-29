#pragma once
#include <SDL.h>

#include "vector2d.h"

// camera is actually stable
// everthing except camera is moving

class Camera {
public:
    ~Camera() = default;
    void setPos(const float &posx, const float &posy);
    static Camera &get();
    vf2d getPos();
    SDL_FRect &getCameraRect();

private:
    Camera(float xoffset, float yoffset, float width, float height);
    static Camera *s_instance;
    float x, y;
    SDL_FRect camera;
};