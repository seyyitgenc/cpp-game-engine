#pragma once
#include "camera.h"

inline void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0,0,width,height);
}

inline void scroll_callback(GLFWwindow *window, double xpos, double ypos)
{
  camera.processMouseScroll(xpos, ypos);
}
inline void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
  camera.processMouseMovement(xpos, ypos);
}

inline  void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}
