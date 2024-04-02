#pragma once

#include "../../external/glad/glad.h"
#include <GLFW/glfw3.h>

class Mouse
{
public:
    static void normalModeCursorPosCallback(GLFWwindow *window, double x, double y);
    static void normalModeMouseButtonCallbaack(GLFWwindow *window, int button, int action, int mods);
    static void normalModeMouseWheelCallback(GLFWwindow *window, double dx, double dy);

    static void editModeCursorPosCallback(GLFWwindow *window, double x, double y);
    static void editModeMouseButtonCallbaack(GLFWwindow *window, int button, int action, int mods);
    static void editModeMouseWheelCallback(GLFWwindow *window, double dx, double dy);


    static double getMouseX();
    static double getMouseY();

    static double getDX();
    static double getDY();

    static double getScrollDX();
    static double getScrollDY();

    static void setFirstMouse(bool firstMouse);
    static bool button(int button);
    static bool buttonChanged(int button);
    static bool buttonWentUp(int button);
    static bool buttonWentDown(int button);

private:
    static double _x;
    static double _y;
    static double _lastX;
    static double _lastY;

    static double _dx;
    static double _dy;

    static double _scrollDX;
    static double _scrollDY;

    static bool _firstMouse;

    static bool _buttons[];
    static bool _buttonsChanged[];
};