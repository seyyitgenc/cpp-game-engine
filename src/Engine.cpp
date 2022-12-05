#include "Engine/Engine.h"
#include "ECS/AssetManager.h"

// Components
#include "ECS/Components/Sprite.h"
#include "ECS/Components/RigidBody.h"
#include "ECS/Components/CollisionBox.h"
#include "ECS/Components/Camera.h"
#include "ECS/Components/UILabel.h"
#include "Engine/Collider.h"

#include <sstream>
#include <random>
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

    // change this SDL_WINDOW RESIZABLE to the SDL_WINDOW_FULLSCREEN
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

std::vector<Entity *> colRects;
Entity *frametimeLabel;
Entity *fpsLabel;
Collider collisionResolver;

// TODO: Engine must consist of only engine related stuff
// TODO: FPS UI label

Clock fpsTimer;
// initialize objects that will be used by game
void Engine::initEntities()
{
    // TODO : Tile map can be done here
    manager = new Manager();

    AssetManager::get().loadFont("aerial", "fonts/aerial.ttf", 16);
    AssetManager::get().loadFont("oswald", "fonts/oswald.ttf", 16);
    AssetManager::get().loadFont("sans", "fonts/sans.ttf", 16);

    AssetManager::get().loadTexture("enemy", "assets/texture.png");
    AssetManager::get().loadTexture("player", "assets/tile.png");

    frametimeLabel = &manager->addEntity();
    frametimeLabel->addComponent<UILabel>(0, 0, "", "sans");
    fpsLabel = &manager->addEntity();
    fpsLabel->addComponent<UILabel>(0, 30, "", "sans");
    // player
    colRects.push_back(&manager->addEntity());
    colRects[0]->addComponent<Sprite>(m_renderer, "player", 64, 64);
    colRects[0]->addComponent<RigidBody>(); // for physics calculation
    colRects[0]->addComponent<CollisionBox>(m_renderer, 64, 64);

    colRects.push_back(&manager->addEntity());
    colRects[1]->addComponent<Sprite>(m_renderer, "enemy", 64, 64);
    colRects[1]->addComponent<CollisionBox>(m_renderer, 128, 128);
    colRects[1]->getComponent<Transform>().position = {300, 300};
    fpsTimer.start();

    // std::mt19937 mt1(time(nullptr));
    // TODO : add entities to specified location
    // for (int i = 2; i < 1000; i++)
    // {
    //     colRects.push_back(&manager->addEntity());
    //     colRects[i]->addComponent<Sprite>(m_renderer, "enemy", 16, 16);
    //     colRects[i]->addComponent<RigidBody>(); // for physics calculation
    //     colRects[i]->addComponent<CollisionBox>(m_renderer, 16, 16);
    //     colRects[i]->getComponent<Transform>().position = {(float)(mt1() % 1200), (float)(mt1() % 700)};
    // }
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
        else if (event.type == SDL_MOUSEBUTTONDOWN)
        {
            if (event.button.button == SDL_BUTTON_LEFT)
            {
            }
        }
    }
}
float Engine::getFrameRate(const int &countedFrames, const Uint64 &fpsTimer)
{
    float avgFps = countedFrames / (fpsTimer / 1000.f);
    if (avgFps > 2000000)
        avgFps = 0;
    return avgFps;
}
void Engine::update(float &dt)
{
    collisionResolver.resolveSweptAABB(colRects, dt);
    manager->update(dt);
}

void Engine::render()
{
    static Uint64 NOW = SDL_GetPerformanceCounter();
    static Uint64 LAST = 0;
    static int countedFrames = 0;
    LAST = NOW;
    NOW = SDL_GetPerformanceCounter();
    // TODO use getDeltaTime to render frameTime
    // frametimeLabel->getComponent<UILabel>().setLabelText("frame time : " + std::to_string((NOW - LAST) / SDL_GetPerformanceFrequency()));
    // fpsLabel->getComponent<UILabel>().setLabelText("FPS : " + std::to_string(getFrameRate(countedFrames, fpsTimer.getTicks())));
    SDL_SetRenderDrawColor(m_renderer, 30, 30, 30, 255);
    SDL_RenderClear(m_renderer);
    manager->draw();
    SDL_RenderPresent(m_renderer);
    countedFrames++;
    // std::cout << getFrameRate(countedFrames, fpsTimer.getTicks()) << std::endl;
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