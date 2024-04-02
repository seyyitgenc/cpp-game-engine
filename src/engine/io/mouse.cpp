#include "mouse.h"

double Mouse::_x = 0;
double Mouse::_y = 0;

double Mouse::_lastX = 0;
double Mouse::_lastY = 0;
double Mouse::_dx = 0;
double Mouse::_dy = 0;

double Mouse::_scrollDX = 0;
double Mouse::_scrollDY = 0;
bool Mouse::_firstMouse = true;

bool Mouse::_buttons[GLFW_MOUSE_BUTTON_LAST] = {0};
bool Mouse::_buttonsChanged[GLFW_MOUSE_BUTTON_LAST] = {0};

// mouse button event
bool Mouse::button(int button){
    return _buttons[button];
}

// mouse button changed event
bool Mouse::buttonChanged(int button){
    bool ret = _buttonsChanged[button];
    _buttonsChanged[button] = false;
    return ret;
}

// mouse button released event
bool Mouse::buttonWentUp(int button){
    return !_buttons[button] && buttonChanged(button);
}

// mouse button pressed event
bool Mouse::buttonWentDown(int button){
    return _buttons[button] && buttonChanged(button);
}

// getter for mouse X
double Mouse::getMouseX(){
    return _x;
}

// getter for mouse Y
double Mouse::getMouseY(){
    return _y;
}

// getter for mouse DX
double Mouse::getDX(){
    double dx = _dx;
    _dx = 0;
    return dx;
}

// getter for mouse DY
double Mouse::getDY(){
    double dy = _dy;
    _dy = 0;
    return dy;
}

// getter for scroll DX
double Mouse::getScrollDX(){
    double dx = _scrollDX;
    _scrollDX = 0;
    return dx;
}

// getter for scroll DY
double Mouse::getScrollDY(){
    double dy =_scrollDY;
   _scrollDY = 0;
    return dy;
}

// setter for firstMouse
void Mouse::setFirstMouse(bool firstMouse){
    _firstMouse = firstMouse;
}

// ---------
// Callbacks
// ---------

// edit mode mouse cursor callback
void Mouse::editModeCursorPosCallback(GLFWwindow *window, double x, double y){

}

// edit mode mouse button callback
void Mouse::editModeMouseButtonCallbaack(GLFWwindow *window, int button, int action, int mods){

}

// edit mode mouse wheel callback
void Mouse::editModeMouseWheelCallback(GLFWwindow *window, double dx, double dy){

}

// normal mode mouse cursor callback
void Mouse::normalModeCursorPosCallback(GLFWwindow *window, double x, double y){
    _x = x;
    _y = y;
    if (_firstMouse){
        _lastX = _x;
        _lastY = _y;
        _firstMouse = false;
    }
    _dx = _x - _lastX;
    _dy = _lastY - _y; // inverted
    _lastX = _x;
    _lastY = _y;
}

// edit mode mouse button callback
void Mouse::normalModeMouseButtonCallbaack(GLFWwindow *window, int button, int action, int mods){
    if (action != GLFW_RELEASE){
        if (_buttons[button])
            _buttons[button] = true;
    }
    else{
        _buttons[button] = false;
    }
    _buttonsChanged[button] = action != GLFW_REPEAT;
}

// edit mode mouse wheel callback
void Mouse::normalModeMouseWheelCallback(GLFWwindow *window, double dx, double dy){
    _scrollDX = dx;
    _scrollDY = dy;
}

