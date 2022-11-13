#include "../include/Engine/Engine.h"
#include "../include/Managers/AssetManager.h"
#include "../include/ECS/Components/Sprite.h"
Engine *Engine::s_instance = nullptr;

Engine::Engine()
{
    m_isRunning = false;
    m_window = nullptr;
    m_renderer = nullptr;
}
Engine::~Engine() {}

void Engine::init()
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

    manager = new Manager();
    auto &e = manager->addEntity();
    auto &text = manager->addEntity();
    AssetManager::get().loadFont("aerial", "fonts/aerial.ttf", 28);
    AssetManager::get().loadTexture("test", "assets/texture.png");
    AssetManager::get().loadRenderedText("text", "test");
    text.addComponent<Sprite>(m_renderer, "text");
    e.addComponent<Sprite>(m_renderer, "test");
}

void Engine::render()
{
    SDL_SetRenderDrawColor(m_renderer, 30, 30, 30, 255);
    SDL_RenderClear(m_renderer);
    manager->draw();
    SDL_RenderPresent(m_renderer);
}

void Engine::update()
{
    manager->update();
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
void Engine::quit()
{
    m_isRunning = false;
}
int Engine::getFrameRate(int countedFrames, Uint32 fpsTimer)
{
    float avgFps = countedFrames / (fpsTimer / 1000.f);
    if (avgFps > 2000000)
        avgFps = 0;
    return avgFps;
}

void Engine::setScreenFps(int SCREEN_FPS, bool isFrameLimitEnabled)
{
    if (isFrameLimitEnabled)
        SCREEN_TICKS_PER_FRAME = 1000.f / SCREEN_FPS;
    else
        SCREEN_TICKS_PER_FRAME = 1000.f / SDL_GetPerformanceCounter();
}
void Engine::limitFrameRate()
{
    m_capTimer.start();
    int frameTicks = m_capTimer.getTicks();
    if (frameTicks < SCREEN_TICKS_PER_FRAME)
        SDL_Delay(SCREEN_TICKS_PER_FRAME - frameTicks);
}