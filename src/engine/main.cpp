#define GLFW_INCLUDE_NONE
#include "engine/app.h"
#include <iostream>

int main() {
    App engine = App::get();
    engine.run();
    engine.clean();
    return 0;
}
