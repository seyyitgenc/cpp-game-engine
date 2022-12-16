#pragma once
#include "Global.h"
#include "vector2d.h"

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
        x = (posx)-SCREEN_WIDTH / 2.0f;
        y = (posy)-SCREEN_HEIGHT / 2.0f;
        if (x < 0)
            x = 0;
        if (y < 0)
            y = 0;
        if (x > LEVEL_WIDTH - camera.w)
            x = LEVEL_WIDTH - camera.w;
        if (y > LEVEL_HEIGHT - camera.h)
            y = LEVEL_HEIGHT - camera.h;
    }
    static Camera &get()
    {
        if (s_instance == nullptr)
        {
            std::cout << "Camera Initialized" << std::endl;
            s_instance = new Camera(0.f, 0.f, SCREEN_WIDTH, SCREEN_HEIGHT);
        }
        return *s_instance;
    }
    vf2d getPos()
    {
        return vf2d(x, y);
    }
    SDL_FRect &getCameraRect() { return camera; };

private:
    static Camera *s_instance;
    float x, y;
    SDL_FRect camera;
};