#pragma once
#include <iostream>
#include <SDL.h>
#include <SDL_mixer.h>
#include "ECS/entitymanager.h"

// TODO : later can this two can change
// max tiles on x axis
#define MAXTILEX 32
// max tiles on y axis
#define MAXTILEY 20

#define SCREEN_WIDTH 1600.0f
#define SCREEN_HEIGHT 900.0f

#define TILE_WIDTH 32.0f
#define TILE_HEIGHT 32.0f

#define LEVEL_WIDTH 1920.0f
#define LEVEL_HEIGHT 1080.0f

#define TOTAL_TILES (MAXTILEX * MAXTILEY)
#define TOTAL_TILE_SPRITES 12

static int SCREEN_FPS = 60;
static int SCREEN_TICKS_PER_FRAME = 1000.0f / SCREEN_FPS;
// Entity Manager
extern Manager *gEntityManager;
// Ground Tiles Manager
extern Manager *gGroundTiles;
// Object Tiles Manager
extern Manager *gPropsTiles;

// UI Manager
extern Manager *gUIManager;

extern SDL_Window *gWindow;     // renderer
extern SDL_Renderer *gRenderer; // window

// Music

extern Mix_Music *gMusic;

bool initGlobals();