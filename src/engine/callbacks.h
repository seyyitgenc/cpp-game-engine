#pragma once
#include "camera.h"
#include "camera_manager.h"
#include "io/mouse.h"

#include "util/helpers.hpp"

inline void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    GNC_UNUSED(window);
    GNC_UNUSED(width);
    GNC_UNUSED(height);

    glViewport(0, 0, width, height);
}

inline void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

inline void setNormalModeCallbacks(GLFWwindow* window)
{
    glfwSetMouseButtonCallback(window, Mouse::normalModeMouseButtonCallback);
    glfwSetCursorPosCallback(window, Mouse::normalModeCursorPosCallback);
    glfwSetScrollCallback(window, Mouse::normalModeMouseWheelCallback);
}
inline void setEditModeCallbacks(GLFWwindow* window)
{
    glfwSetMouseButtonCallback(window, Mouse::editModeMouseButtonCallback);
    glfwSetCursorPosCallback(window, Mouse::editModeCursorPosCallback);
    glfwSetScrollCallback(window, Mouse::editModeMouseWheelCallback);
}