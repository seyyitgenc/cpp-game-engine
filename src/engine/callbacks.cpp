#include "callbacks.h"

#include "glad/glad.h"

#include <GLFW/glfw3.h>

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    GNC_UNUSED(window);
    GNC_UNUSED(width);
    GNC_UNUSED(height);

    // TODO: resize frame buffers madafaka
    GNC_UNUSED("Resize frame buffers here!");

    glViewport(0, 0, width, height);
}

void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

void setNormalModeCallbacks(GLFWwindow* window)
{
    glfwSetMouseButtonCallback(window, Mouse::normalModeMouseButtonCallback);
    glfwSetCursorPosCallback(window, Mouse::normalModeCursorPosCallback);
    glfwSetScrollCallback(window, Mouse::normalModeMouseWheelCallback);
}
void setEditModeCallbacks(GLFWwindow* window)
{
    glfwSetMouseButtonCallback(window, Mouse::editModeMouseButtonCallback);
    glfwSetCursorPosCallback(window, Mouse::editModeCursorPosCallback);
    glfwSetScrollCallback(window, Mouse::editModeMouseWheelCallback);
}