#pragma once
#include <glad/glad.h>
#include "shader.h"
#include "model.h"

class OpenglObject
{
public:
    // TODO:
    // note: or i can call model loading function here and can accept indices paths etc...
    // this will accept incoming model
    OpenglObject(Model  m) {
        _m = m;
    }
private:
    Model _m;
};
