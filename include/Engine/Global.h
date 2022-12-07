#pragma once
#include "SDL.h"

const float SCREEN_WIDTH = 800;
const float SCREEN_HEIGHT = 600;

const int LEVEL_WIDTH = 1600;
const int LEVEL_HEIGHT = 1200;

class Camera
{
public:
    Camera(int xoffset, int yoffset, int width, int height)
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
    SDL_Rect &getCameraRect() { return camera; };

private:
    float x, y;
    SDL_Rect camera;
};

static Camera playerCam(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);