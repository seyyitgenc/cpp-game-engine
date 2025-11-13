#pragma once
#include <iostream>

#include "external/ImGui/imgui.h"
#include "external/ImGui/imgui_impl_glfw.h"
#include "external/ImGui/imgui_impl_opengl3.h"

#include "util/filesystem.hpp"
#include "util/log.hpp"

#include "callbacks.h"
#include "camera_manager.h"
#include "io/keyboard.h"
#include "io/mouse.h"
#include "texture.h"

#include <GLFW/glfw3.h>

constexpr int SCREEN_WIDTH = 1280;
constexpr int SCREEN_HEIGHT = 720;
constexpr int SHADOW_WIDTH = 2160;
constexpr int SHADOW_HEIGHT = 2160;
constexpr float THUMBNAIL_SIZE = 128.0f;
constexpr int TILE_WIDTH = 32;
constexpr int TILE_HEIGHT = 32;

extern GLFWwindow* gWindow;
extern bool gEditModeEnabled;

struct Viewport {
    int _width = SCREEN_WIDTH, _height = SCREEN_HEIGHT;
    int posx = 0, posy = 0;
};

inline Viewport gViewport;
extern bool gInitGlobals();