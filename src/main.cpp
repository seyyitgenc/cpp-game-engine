#define GLFW_INCLUDE_NONE
#include "Engine/engine.h"
#include <iostream>

int main() {
    Engine engine = Engine::get();
    engine.run();
    engine.clean();
    return 0;
}
