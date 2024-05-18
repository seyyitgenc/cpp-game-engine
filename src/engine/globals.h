#pragma once
#include <iostream>

#include "../external/ImGui/imgui.h"
#include "../external/ImGui/imgui_impl_glfw.h"
#include "../external/ImGui/imgui_impl_opengl3.h"
#include "../util/filesystem.hpp"
#include "../util/log.hpp"

#include "texture.h"
#include "shader_manager.h"
#include "camera_manager.h"
#include "callbacks.h"
#include "io/keyboard.h"
#include "io/mouse.h"
#include <GLFW/glfw3.h>

constexpr int SCREEN_WIDTH  = 1280;
constexpr int SCREEN_HEIGHT = 720;
constexpr int SHADOW_WIDTH = 2160;
constexpr int SHADOW_HEIGHT = 2160;
constexpr float THUMBNAIL_SIZE = 128.0f;
constexpr int TILE_WIDTH  =  32;
constexpr int TILE_HEIGHT = 32;

extern GLFWwindow *gWindow;
extern bool gEditModeEnabled;

extern TextureManager   *gTextureManager;
extern ShaderManager    *gShaderManager;
extern CameraManager    *gCameraManager;

extern bool gInitGlobals();