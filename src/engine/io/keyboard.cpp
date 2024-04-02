#include "keyboard.h"

bool Keyboard::_keys[GLFW_KEY_LAST] = {0};
bool Keyboard::_keysChanged[GLFW_KEY_LAST] = {0};

// keyboard default key callback
void Keyboard::keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods){
    if (action != GLFW_RELEASE){
        if (!_keys[key])
            _keys[key] = true;
    }
    else{
        _keys[key] = false;
    }
    _keysChanged[key] = action != GLFW_REPEAT;
}

// keyboard key event
bool Keyboard::key(int key){
    return _keys[key];
}

// keyboard key changed event
bool Keyboard::keyChanged(int key){
    bool ret = _keysChanged[key];
    _keysChanged[key] = false;
    return ret;
}

// keyboard key released event
bool Keyboard::keyWentUp(int key){
    return !_keys[key] && keyChanged(key);
}

// keyboard key pressed event
bool Keyboard::keyWentDown(int key){
    return _keys[key] && keyChanged(key);
}