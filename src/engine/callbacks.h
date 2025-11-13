#pragma once
#include "camera.h"
#include "camera_manager.h"
#include "io/mouse.h"

#include "util/helpers.hpp"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void glfw_error_callback(int error, const char* description);
void setNormalModeCallbacks(GLFWwindow* window);
void setEditModeCallbacks(GLFWwindow* window);