#pragma once
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <iostream>

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_glfw.h"
#include "ImGui/imgui_impl_opengl3.h"

#include "shader_manager.h"
#include "util/filesystem.h"

#define SCREEN_WIDTH  1366.0f
#define SCREEN_HEIGHT 768.0f

#define TILE_WIDTH  32.0f
#define TILE_HEIGHT 32.0f

extern GLFWwindow *gWindow;
extern bool gEditModeEnabled;

extern ShaderManager *gShaderManager;


extern bool gInitGlobals();
extern void gInitShaders();