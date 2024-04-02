#pragma once

#include "light.h"
#include "globals.h"
#include "renderer/deferred_renderer.h"

#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

class App {
private:
public:
    App();
    ~App();

    // ensures that app created only once a.k.a : Singleton
    static App &getInstance() {
        if (_instance == nullptr) {
            _instance = new App();
            Log::write(
                Log::Info,
                GREEN_TEXT("INFO::APP::GET_INSTANCE App Initialized with mem address -> "),
                &_instance,
                "\n");
        }
        return *_instance;
    }
    // todo: find a good way to resolve this.
    // fixme: possibly nullptr when called
    void run();
    void clean();
    void initEntities();

    void render();
    void update(const float &dt);
    void processInput(GLFWwindow* window);
private:
    // App Field
    ImVec4 clear_color = ImVec4(0.45f, 0.55f,0.60f, 1.00f);
    static App *_instance;
    DeferredRenderer *_deferredRenderer = nullptr;
};