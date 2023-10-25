#pragma once
#include <glad/glad.h>
#include "shader.h"
#include "model.h"

class Object
{
public:
    // void translate
private:
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;

};