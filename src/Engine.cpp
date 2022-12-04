#include "Engine/Engine.h"
#include "ECS/AssetManager.h"

// Components
#include "ECS/Components/Sprite.h"
#include "ECS/Components/RigidBody.h"
#include "ECS/Components/CollisionBox.h"
#include "ECS/Components/Collider.h"
#include "ECS/Components/UILabel.h"

#include <sstream>

Engine *Engine::s_instance;

// TODO : move cpp files to it's related paths
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
// TODO organize these object
Entity *p1;
Entity *p2;

std::vector<Entity *> colRects;
Entity *label;

Collider test;

// initialize objects that will be used by game
void Engine::initEntities()
{
    // TODO : Tile map can be done here
    manager = new Manager();
    AssetManager::get().loadFont("aerial", "fonts/aerial.ttf", 16);
    AssetManager::get().loadTexture("enemy", "assets/texture.png");
    AssetManager::get().loadTexture("player", "assets/tile.png");
    SDL_Color red = {255, 0, 0, 255};
    label = &manager->addEntity();
    label->addComponent<UILabel>(0, 0, "my new text", "aerial", red);

    // player
    colRects.push_back(&manager->addEntity());
    colRects[0]->addComponent<Sprite>(m_renderer, "player", 64, 64);
    colRects[0]->addComponent<RigidBody>(); // for physics calculation
    colRects[0]->addComponent<CollisionBox>(m_renderer, 32, 32);

    colRects.push_back(&manager->addEntity());
    colRects[1]->addComponent<Sprite>(m_renderer, "enemy", 64, 64);
    colRects[1]->addComponent<CollisionBox>(m_renderer, 128, 128);
    colRects[1]->getComponent<Transform>().position = {300, 300};
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
    test.resolveSweptAABB(colRects, dt);
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