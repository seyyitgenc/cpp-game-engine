#pragma once
#include "../component.h"
#include "Engine/vector2d.h"

struct Transform : public Component {
    Transform(float x, float y) {
        position.x = x;
        position.y = y;
        scale.x = 1;
        scale.y = 1;
    };
    Transform(float x, float y, float scx, float scy, float rot) {
        position.x = x;
        position.y = y;
        scale.x = scx;
        scale.y = scy;
        rotation = rot;
    };
    ~Transform() = default;
    vf2d position;
    vf2d scale;
    float rotation = 0.0f;
};
