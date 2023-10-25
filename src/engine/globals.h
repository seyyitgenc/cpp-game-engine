#pragma once
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <iostream>

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_glfw.h"
#include "ImGui/imgui_impl_opengl3.h"

#include "shader_manager.h"

#define SCREEN_WIDTH  800.0f
#define SCREEN_HEIGHT 600.0f

#define TILE_WIDTH  32.0f
#define TILE_HEIGHT 32.0f

extern GLFWwindow *gWindow;
extern bool gEditModeEnabled;

// extern ShaderManager *gShaderManager;

bool initGlobals();