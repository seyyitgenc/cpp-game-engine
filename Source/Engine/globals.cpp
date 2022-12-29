#include "globals.h"

#include "Engine/tilemanager.h"

// Window and renderer
SDL_Window *gWindow;
SDL_Renderer *gRenderer;

// Managers
Manager *gEntityManager;
Manager *gGroundTiles;
Manager *gUIManager;
Manager *gPropsTiles;

Mix_Music *gMusic;

// Global variable initalization
bool initGlobals() {
    // Initialize Managers
    gEntityManager = new Manager();
    gGroundTiles = new Manager();
    gUIManager = new Manager();
    gPropsTiles = new Manager();

    // initialize window and renderer
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        std::cerr << SDL_GetError() << std::endl;
        return false;
    }
    // change this SDL_WINDOW RESIZABLE to the SDL_WINDOW_FULLSCREEN
    auto wflags = (SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

    gWindow =
        SDL_CreateWindow("Game Engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, wflags);
    if (!gWindow) {
        std::cout << "Window creation error!" << std::endl;
        return false;
    }

    auto rflags = (SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    gRenderer = SDL_CreateRenderer(gWindow, -1, rflags);
    SDL_SetRenderDrawBlendMode(gRenderer, SDL_BLENDMODE_BLEND);
    if (!gRenderer) {
        std::cout << "Renderer creation error!" << std::endl;
        return false;
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) != 0) {
        std::cout << "SDL_mixer could not initialize! SDL_mixer Error : " << Mix_GetError() << std::endl;
    }

    // load tiles
    if (!TileManager::get().loadTiles("assets/Tileset-Grass.png", 2.0f, 2.0f))
        std::cout << "Error occured while loading Tiles" << std::endl;

    gMusic = Mix_LoadMUS("resources/beat.wav");
    if (gMusic == NULL) {
        std::cout << "failed to load music" << std::endl;
    }
    return true;
}