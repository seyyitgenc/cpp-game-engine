#pragma once
#include <iostream>
#include <map>

#include "globals.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


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
    void clean();
    void initEntities();

    void render();
    void update(const float &dt);
    void processInput();

private:
    // Engine Field
    static Engine *s_instance;
private:
};