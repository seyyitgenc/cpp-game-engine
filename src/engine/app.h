#pragma once

#include <iostream>
#include <map>
#include "object.h"
#include "globals.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


#include "camera.h"

class App {
private:
public:
    App();
    ~App();

    // ensures that engine created only once Singleton
    static App &get() {
        if (s_instance == nullptr) {
            std::cout << "App Created" << std::endl;
            s_instance = new App();
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
    // App Field
    static App *s_instance;
private:
};

// TODO : this class can contain opengl objects container. Later on, this container will contain opengl objects that have several functions like : draw , update etc..