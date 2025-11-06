#pragma once

#include "globals.h"
#include "light.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

class App {
private:
public:
    App();
    ~App();

    // ensures that app created only once a.k.a : Singleton
    static App& getInstance()
    {
        if (_instance == nullptr) {
            _instance = new App();
            Log::writeFormatted(
                Log::Info,
                GREEN_TEXT(
                    "INFO::APP::GET_INSTANCE App Initialized with mem address -> %p")
                    .c_str(),
                &_instance);
        }
        return *_instance;
    }
    // todo: find a good way to resolve this.
    // fixme: possibly nullptr when called
    void run();
    void clean();

    void render();
    void update(const float& dt);
    void processInput(GLFWwindow* window);

private:
    // App Field
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    static App* _instance;
};