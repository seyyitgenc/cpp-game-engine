#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>

#define SCREEN_WIDTH  800.0f
#define SCREEN_HEIGHT 600.0f

#define TILE_WIDTH  32.0f
#define TILE_HEIGHT 32.0f

extern GLFWwindow *gWindow;

bool initGlobals();