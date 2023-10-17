#pragma once
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <iostream>

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_glfw.h"
#include "ImGui/imgui_impl_opengl3.h"

#define SCREEN_WIDTH  1400.0f
#define SCREEN_HEIGHT 900.0f

#define TILE_WIDTH  32.0f
#define TILE_HEIGHT 32.0f

extern GLFWwindow *gWindow;
extern bool gEditModeEnabled;


bool initGlobals();