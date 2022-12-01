#include "Engine/Engine.h"
#include "ECS/AssetManager.h"

#include "ECS/Components/Sprite.h"
#include "ECS/Components/RigidBody.h"
#include "ECS/Components/CollisionBox.h"
#include "headers/Player.h"

Engine *Engine::s_instance;

Engine::Engine()
{
    m_isRunning = false;
    m_window = nullptr;
    m_renderer = nullptr;
}
Engine::~Engine() {}

// initialize window and renderer
void Engine::initApp()
{
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
        std::cerr << SDL_GetError() << std::endl;
    auto wflags = (SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    m_window = SDL_CreateWindow("Game Engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, wflags);
    if (!m_window)
        std::cout << "Window creation error!" << std::endl;
    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED);
    if (!m_renderer)
        std::cout << "Renderer creation error!" << std::endl;
    m_clearColor = DARK;
    m_isRunning = true;

    fpsTimer.start();
}
// initialize objects that will be used by game
void Engine::initEntities()
{
    manager = new Manager();
    // TODO : Tile map can be done here
    AssetManager::get().loadFont("aerial", "fonts/aerial.ttf", 28);
    
    AssetManager::get().loadTexture("texture", "assets/texture.png");
    AssetManager::get().loadTexture("player", "assets/tile.png");

    // player
    auto &p1 = manager->addEntity();
    p1.addComponent<Sprite>(m_renderer, "player", 64, 64);
    p1.addComponent<RigidBody>();
    p1.addComponent<CollisionBox>(m_renderer, 96, 128);
}

void Engine::events()
{
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0)
    {
        if (event.type == SDL_QUIT)
            m_isRunning = false;
        if (event.type == SDL_WINDOWEVENT_RESIZED)
        {
        }
        else if (event.type == SDL_KEYDOWN)
        {
        }
    }
}
void Engine::update(float &dt)
{
    manager->update(dt);
}

void Engine::render()
{
    static int countedFrames = 0;
    SDL_SetRenderDrawColor(m_renderer, 30, 30, 30, 255);
    SDL_RenderClear(m_renderer);
    manager->draw();
    SDL_RenderPresent(m_renderer);
    countedFrames++;
}

void Engine::clean()
{
    AssetManager::get().clean();
    SDL_DestroyRenderer(m_renderer);
    SDL_DestroyWindow(m_window);
    SDL_Quit();
    TTF_Quit();
    IMG_Quit();
}

void Engine::quit()
{
    m_isRunning = false;
}
