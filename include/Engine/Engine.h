#pragma once
#include <iostream>
#include "SDL.h"
#include "ECS/EntityManager.h"
#include "Clock.h"
#include <map>

struct rect
{
    vf2d pos;
    vf2d size;
    vf2d vel;
};
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const SDL_Color DARK = {30, 30, 30, 255};

class Engine
{
private:
public:
    Engine();
    ~Engine();

    // ensures that engine created only once Singleton
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
    void clean();
    void initApp();
    void initEntities();

    void render();
    void update(float &dt);
    void events();
    bool isRunning() { return m_isRunning; }

    SDL_Renderer *getRenderer() { return m_renderer; };

    Clock fpsTimer;

private:
    Manager *manager;
    static Engine *s_instance;
    bool m_isRunning;
    SDL_Renderer *m_renderer;
    SDL_Window *m_window;
    SDL_Color m_clearColor;
};
