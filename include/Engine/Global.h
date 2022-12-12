#pragma once
#include "SDL.h"
#include "vector2d.h"

const float SCREEN_WIDTH = 800;
const float SCREEN_HEIGHT = 600;

const int LEVEL_WIDTH = 1600;
const int LEVEL_HEIGHT = 1200;

// camera is actually stable
// everthing except camera is moving
class Camera
{
public:
    Camera(float xoffset, float yoffset, float width, float height)
    {
        camera =
            {xoffset,
             yoffset,
             width,
             height};
    }

    ~Camera() = default;
    void setPos(float posx, float posy)
    {
        x = (posx + 16.0f) - SCREEN_WIDTH / 2.0f;
        y = (posy + 32.0f) - SCREEN_HEIGHT / 2.0f;
        if (x < 0)
            x = 0;
        if (y < 0)
            y = 0;
        if (x + camera.w > LEVEL_WIDTH)
            x = camera.w;
        if (y + camera.h > LEVEL_HEIGHT)
            y = camera.h;
    }
    vf2d getPos()
    {
        return vf2d(x, y);
    }
    SDL_FRect &getCameraRect() { return camera; };

private:
    float x, y;
    SDL_FRect camera;
};

static Camera playerCam(0.f, 0.f, SCREEN_WIDTH, SCREEN_HEIGHT);