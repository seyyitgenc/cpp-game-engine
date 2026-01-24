#pragma once

#include "ImGui/imgui.h"

struct GLFWwindow;

namespace GNC {

class Scene;

class App {
public:
    App();
    ~App();

    // singleton
    static App& instance();

    // todo: find a good way to resolve this.
    // fixme: possibly nullptr when called
    void run();
    void clean();

    void render();
    void update(const float& dt);
    void processInput(GLFWwindow* window);

private:
    // App Field
    ImVec4 clear_color;
    static App* _instance;

    Scene* scene = nullptr;
};

} // namespace GNC