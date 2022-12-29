#pragma once
#include <iostream>
#include <map>

#include "camera.h"
#include "clock.h"
#include "globals.h"
#include "utils.h"

// TODO try to draw collision rect alongside with sprite

const SDL_Color rendererColor = {30, 30, 30, 255};

class Engine {
private:
public:
    Engine();
    ~Engine();

    // ensures that engine created only once Singleton
    static Engine &get() {
        if (s_instance == nullptr) {
            std::cout << "Engine Created" << std::endl;
            s_instance = new Engine();
        }
        return *s_instance;
    }
    void run();
    void quit();
    void clean();
    void initEntities();

    void render();
    void update(const float &dt);
    void events();
    bool isRunning() { return m_isRunning; }

    Clock fpsTimer;

private:
    // Engine Field
    static Engine *s_instance;
    bool m_isRunning;

private:
    Clock m_capTimer;
};