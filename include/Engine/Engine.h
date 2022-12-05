#pragma once
#include <iostream>
#include "vector2d.h"
#include "SDL.h"
#include "ECS/EntityManager.h"
#include "Clock.h"
#include <map>

// TODO merge game and engine headers
// TODO try to draw collision rect alongside with sprite

const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 960;

const int LEVEL_WIDTH = 1280;
const int LEVEL_HEIGHT = 960;

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

    float getFrameRate(const int &countedFrames, const Uint64 &fpsTimer);

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