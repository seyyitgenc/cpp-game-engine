#pragma once

#include <iostream>
#include "object.h"
#include "globals.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class App {
private:
public:
    App();
    ~App();

    // ensures that app created only once a.k.a : Singleton
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
    void processInput(GLFWwindow* window);

private:
    //fixme: temporary class
    bool is_j_pressed = false;
    // App Field
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f,0.60f, 1.00f);
    
    static App *s_instance;
private:
};