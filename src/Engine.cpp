#include "Engine/Engine.h"
#include "ECS/AssetManager.h"

// Components
#include "ECS/Components/Sprite.h"
#include "ECS/Components/RigidBody.h"
#include "ECS/Components/CollisionBox.h"
#include "ECS/Components/UILabel.h"
#include "Engine/Collider.h"
#include "Engine/KeybordManager.h"

#include <sstream>
#include <random>

Engine *Engine::s_instance;

Engine::Engine()
{
    m_isRunning = false;
    m_window = nullptr;
    m_renderer = nullptr;
}
Engine::~Engine()
{
    this->clean();
}

// initialize window and renderer
void Engine::initApp()
{
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
        std::cerr << SDL_GetError() << std::endl;

    // change this SDL_WINDOW RESIZABLE to the SDL_WINDOW_FULLSCREEN
    auto wflags = (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_ALLOW_HIGHDPI);
    m_window = SDL_CreateWindow("Game Engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, wflags);
    if (!m_window)
        std::cout << "Window creation error!" << std::endl;
    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED);
    if (!m_renderer)
        std::cout << "Renderer creation error!" << std::endl;
    m_isRunning = true;

    fpsTimer.start();
}

std::vector<Entity *> colRects;
Entity *frametimeLabel;
Entity *fpsLabel;
Entity *cameraPos;
Entity *playerPos;
Collider collisionResolver;

Clock fpsTimer;

// initialize objects that will be used by g"ame
void Engine::initEntities()
{
    // TODO : Tile map can be done here
    manager = new Manager();

    // load fonts
    AssetManager::get().loadFont("aerial", "fonts/aerial.ttf", 16);
    AssetManager::get().loadFont("oswald", "fonts/oswald.ttf", 16);
    AssetManager::get().loadFont("sans", "fonts/sans.ttf", 16);

    // load textures
    AssetManager::get().loadTexture("test", "assets/texture.png");
    AssetManager::get().loadTexture("tile", "assets/tile.png");
    AssetManager::get().loadTexture("player", "assets/player.png");
    AssetManager::get().loadTexture("enemy", "assets/enemy.png");

    // UI Elements
    frametimeLabel = &manager->addEntity();
    frametimeLabel->addComponent<UILabel>(0, 0, "", "sans");
    fpsLabel = &manager->addEntity();
    fpsLabel->addComponent<UILabel>(0, 30, "", "sans");
    cameraPos = &manager->addEntity();
    cameraPos->addComponent<UILabel>(0, 60, "", "sans");
    playerPos = &manager->addEntity();
    playerPos->addComponent<UILabel>(0, 90, "", "sans");

    // player
    colRects.push_back(&manager->addEntity());
    colRects[0]->addComponent<Sprite>(m_renderer, "player", 64, 128);
    colRects[0]->addComponent<RigidBody>(); // for physics calculation
    colRects[0]->addComponent<CollisionBox>(m_renderer, 50, 110);

    // enemy
    colRects.push_back(&manager->addEntity());
    colRects[1]->addComponent<Sprite>(m_renderer, "enemy", 128, 128);
    colRects[1]->addComponent<CollisionBox>(m_renderer, 110, 94);
    colRects[1]->getComponent<Transform>().position = {900, 400};
    fpsTimer.start();

    // std::mt19937 mt1(time(nullptr));
    // for (int i = 2; i < 4900; i++)
    // {
    //     colRects.push_back(&manager->addEntity());
    //     colRects[i]->addComponent<Sprite>(m_renderer, "enemy", 16, 16);
    //     colRects[i]->addComponent<RigidBody>(); // for physics calculation
    //     colRects[i]->addComponent<CollisionBox>(m_renderer, 16, 16);
    //     colRects[i]->getComponent<Transform>().position = {(float)(mt1() % 900), (float)(mt1() % 900)};
    // }
}

// main game loop
void Engine::run()
{
    this->initApp();
    this->initEntities();
    while (this->isRunning())
    {
        this->events();
        this->update(this->getDeltaTime());
        this->render();
    }
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

void Engine::update(float &dt)
{

    collisionResolver.resolveSweptAABB(colRects, dt);
    manager->update(dt);
}

void Engine::render()
{
    static int countedFrames = 0;
    countedFrames++;
    // UI
    frametimeLabel->getComponent<UILabel>().setLabelText("frame time : " + std::to_string(getDeltaTime()));
    fpsLabel->getComponent<UILabel>().setLabelText("FPS : " + std::to_string(getFrameRate(countedFrames, fpsTimer.getTicks())));
    cameraPos->getComponent<UILabel>().setLabelText("Cam Pos : " + std::to_string(playerCam.getPos().x) + " , " + std::to_string(playerCam.getPos().y));
    playerPos->getComponent<UILabel>().setLabelText("Player Pos : " + std::to_string(colRects[0]->getComponent<Transform>().position.x) + " , " + std::to_string(colRects[0]->getComponent<Transform>().position.y));

    // for camera position
    playerCam.setPos(colRects[0]->getComponent<Transform>().position.x, colRects[0]->getComponent<Transform>().position.y);
    SDL_SetRenderDrawColor(m_renderer, rendererColor.r, rendererColor.g, rendererColor.b, rendererColor.a);
    SDL_RenderClear(m_renderer);
    manager->draw();
    // Set Camera Color
    SDL_SetRenderDrawColor(m_renderer, 0, 255, 0, 255);
    // Draw Camera Rectangle
    SDL_RenderDrawRect(m_renderer, &playerCam.getCameraRect());
    SDL_RenderPresent(m_renderer);
}

float &Engine::getDeltaTime()
{
    static Uint64 NOW = SDL_GetPerformanceCounter();
    static Uint64 LAST = 0;
    static float dt;
    LAST = NOW;
    NOW = SDL_GetPerformanceCounter();
    dt = (NOW - LAST) / (double)SDL_GetPerformanceFrequency();
    return dt;
}

void Engine::setFrameRate(const int &SCREEN_FPS)
{
    SCREEN_TICKS_PER_FRAME = 1000.0f / SCREEN_FPS;
}

void Engine::limitFrameRate()
{
    m_capTimer.start();
    int frameTicks = m_capTimer.getTicks();
    if (frameTicks < SCREEN_TICKS_PER_FRAME)
        SDL_Delay(SCREEN_TICKS_PER_FRAME - frameTicks);
}

float Engine::getFrameRate(const int &countedFrames, const Uint64 &fpsTimer)
{
    float avgFps = countedFrames / (fpsTimer / 1000.f);
    if (avgFps > 2000000)
        avgFps = 0;
    return avgFps;
}

void Engine::clean()
{
    // TODO destroy colrects entities
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