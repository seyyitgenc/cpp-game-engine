#pragma once
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <iostream>

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_glfw.h"
#include "ImGui/imgui_impl_opengl3.h"

#include "shader_manager.h"
#include "texture.h"
#include "util/filesystem.hpp"
#include "util/log.hpp"

constexpr int SCREEN_WIDTH  = 1366;
constexpr int SCREEN_HEIGHT = 768;
constexpr float THUMBNAIL_SIZE = 128.0f;
constexpr int TILE_WIDTH  =  32;
constexpr int TILE_HEIGHT = 32;

extern GLFWwindow *gWindow;
extern bool gEditModeEnabled;

extern ShaderManager *gShaderManager;


extern bool gInitGlobals();
extern void gInitShaders();
extern void gInitTextures();