#pragma once
#include <GLFW/glfw3.h>
#include "camera.h"

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0,0,width,height);
}

void scroll_callback(GLFWwindow *window, double xpos, double ypos)
{
    
}
void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
}