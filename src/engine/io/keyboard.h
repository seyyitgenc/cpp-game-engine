#pragma once

#include "../../external/glad/glad.h"
#include <GLFW/glfw3.h>

class Keyboard
{
public:
    // key state callback
    static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);

    // accessors

    static bool key(int key);
    static bool keyChanged(int key);
    static bool keyWentUp(int key);
    static bool keyWentDown(int key);

private:
    static bool _keys[];
    static bool _keysChanged[];
};