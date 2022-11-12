#pragma once
#include "Component.h"
#ifndef vector_h
#define vector_h
#include "../vector2d.h"
#endif
class Transform : public Component
{
public:
    Transform() = default;
    Transform(float x, float y)
    {
        position.x = x;
        position.y = y;
    };
    Transform(float x, float y, float scx, float scy, float rot)
    {
        position.x = x;
        position.y = y;
        scale.x = scx;
        scale.y = scy;
        rotation = rot;
    };
    ~Transform() = default;

    vf2d position = vf2d();
    vf2d scale = vf2d(1, 1);
    float rotation = 0.0f;
};
