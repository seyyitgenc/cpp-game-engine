#pragma once
#include <iostream>
#include "../SDL2/SDL.h"
#include "../ECS/Entity/EntityManager.h"
#include "Clock.h"

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const SDL_Color DARK = {30, 30, 30, 255};

class Engine
{
private:
public:
    Engine();
    ~Engine();

    // ensures that engine created only once
    static Engine &get()
    {
        if (s_instance == nullptr)
        {
            std::cout << "Engine Created" << std::endl;
            s_instance = new Engine();
        }
        return *s_instance;
    }

    void quit();
    void init();
    void clean();

    void setScreenFps(int, bool);  // set screen fps
    int getFrameRate(int, Uint32); // get frame rate
    void limitFrameRate();

    void render();
    void update();
    void events();
    bool isRunning() { return m_isRunning; }

    SDL_Renderer *getRenderer() { return m_renderer; };

private:
    static Engine *s_instance;
    Manager *manager;

    bool m_isRunning;
    SDL_Renderer *m_renderer;
    SDL_Window *m_window;
    SDL_Color m_clearColor;

    // Game Tick and FPS field
    int SCREEN_FPS = 120;
    int SCREEN_TICKS_PER_FRAME = 1000.f / SCREEN_FPS;
    // Timer field
    Clock m_capTimer;
    unsigned int m_countedFrames = 0;
};
