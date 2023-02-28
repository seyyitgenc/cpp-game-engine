#include "Engine/engine.h"

int main(int argc, char **argv) {
    Engine *engine;
    engine = &Engine::get();
    engine->run();  // main game loop
    engine->~Engine();
    return EXIT_SUCCESS;
}
