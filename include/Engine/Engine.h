#pragma once
#include <iostream>
#include "vector2d.h"
#include "SDL.h"
#include "ECS/EntityManager.h"
#include "Clock.h"
#include "Global.h"
#include <map>

// TODO try to draw collision rect alongside with sprite

const SDL_Color rendererColor = {30, 30, 30, 255};

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
    void run();
    void quit();
    void clean();
    void initApp();
    void initEntities();

    float &getDeltaTime();
    float getFrameRate(const int &countedFrames, const Uint64 &fpsTimer);
    void setFrameRate(const int &SCREEN_FPS);
    void limitFrameRate();
    void render();
    void update(float &dt);
    void events();
    bool isRunning() { return m_isRunning; }

    SDL_Renderer *getRenderer() { return m_renderer; };

    Clock fpsTimer;

private:
    // Engine Field
    Manager *manager;
    static Engine *s_instance;
    bool m_isRunning;
    SDL_Renderer *m_renderer;
    SDL_Window *m_window;

private:
    // Game Field
    int SCREEN_FPS = 70;
    int SCREEN_TICKS_PER_FRAME = 1000.0f / SCREEN_FPS;
    Clock m_capTimer;
};